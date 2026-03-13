#include "reportgenerator.h"
#include "clinicsettings.h"
#include <QTextDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QWidget>
#include <QDate>
#include <QDateTime>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>
#include <QBuffer>
#include <QPixmap>

static QString escape(const QString& s) {
    QString r = s;
    r.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;");
    return r;
}

static QString cssBase()
{
    return R"(
    <style>
    * { margin:0; padding:0; box-sizing:border-box; }
    body { font-family:'Segoe UI',Arial,sans-serif; font-size:10pt; color:#212121;
           background:white; padding:20px; }
    .header { border-bottom:3px solid #1565C0; padding-bottom:12px; margin-bottom:16px;
              display:flex; justify-content:space-between; }
    .clinic-name { font-size:20pt; font-weight:bold; color:#1565C0; }
    .clinic-sub  { font-size:9pt; color:#757575; margin-top:4px; }
    .print-date  { font-size:9pt; color:#9E9E9E; text-align:right; }
    h2 { font-size:14pt; color:#1565C0; margin:12px 0 8px; border-bottom:1px solid #E0E0E0; padding-bottom:4px; }
    h3 { font-size:11pt; color:#00695C; margin:10px 0 6px; }
    .info-table  { width:100%; border-collapse:collapse; margin-bottom:12px; }
    .info-table td { padding:5px 8px; vertical-align:top; }
    .info-table td:first-child { font-weight:bold; color:#555; width:30%; }
    .info-table tr:nth-child(even) td { background:#F9F9F9; }
    .iv-card { border:1px solid #E0E0E0; border-radius:6px; margin-bottom:12px;
               page-break-inside:avoid; }
    .iv-header { background:#1565C0; color:white; padding:8px 12px; border-radius:5px 5px 0 0;
                 font-weight:bold; font-size:10pt; }
    .iv-body   { padding:10px 12px; }
    .iv-body table { width:100%; border-collapse:collapse; }
    .iv-body td { padding:4px 6px; vertical-align:top; }
    .iv-body td:first-child { font-weight:bold; color:#555; width:30%; }
    .iv-body tr:nth-child(even) td { background:#F9F9F9; }
    .cost-badge { display:inline-block; background:#FFF9C4; border:1px solid #F9A825;
                  border-radius:4px; padding:2px 8px; font-weight:bold; color:#F57F17; }
    .status-completed { color:#2E7D32; font-weight:bold; }
    .status-scheduled { color:#1565C0; font-weight:bold; }
    .status-cancelled { color:#C62828; font-weight:bold; }
    .pdf-note { background:#E3F2FD; border-left:3px solid #1565C0; padding:6px 10px;
                margin-top:8px; font-size:9pt; color:#1565C0; }
    .sig-section { margin-top:8px; }
    .sig-line { border-bottom:1px solid #424242; width:200px; margin:20px 0 4px; }
    .footer { border-top:1px solid #E0E0E0; margin-top:20px; padding-top:8px;
              font-size:8pt; color:#9E9E9E; text-align:center; }
    .summary-box { border:1px solid #C8E6C9; background:#F1F8E9; border-radius:6px;
                   padding:10px 16px; margin-bottom:16px; }
    .summary-box td { padding:3px 12px 3px 0; }
    .summary-box td:first-child { font-weight:bold; color:#33691E; }
    </style>
    )";
}

static QString headerHtml()
{
    auto& cs = ClinicSettings::instance();

    QString logoHtml;
    if (cs.hasLogo()) {
        // Embed logo as base64 so it prints correctly
        QPixmap px = cs.logo();
        if (!px.isNull()) {
            QByteArray ba;
            QBuffer buf(&ba);
            buf.open(QIODevice::WriteOnly);
            px.scaled(180, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation)
              .save(&buf, "PNG");
            buf.close();
            logoHtml = "<img src='data:image/png;base64," +
                       ba.toBase64() +
                       "' style='max-height:60px;max-width:180px;"
                       "vertical-align:middle;'/>";
        }
    }

    QString contactLine;
    QStringList parts;
    if (!cs.clinicAddress.isEmpty()) parts << cs.clinicAddress;
    if (!cs.clinicPhone.isEmpty())   parts << "Tel: " + cs.clinicPhone;
    if (!cs.clinicEmail.isEmpty())   parts << cs.clinicEmail;
    if (!parts.isEmpty())
        contactLine = "<div style='font-size:8pt;color:#757575;margin-top:2px;'>"
                      + escape(parts.join("  |  ")) + "</div>";

    return QString(R"(
    <div class='header'>
      <div style='display:flex;align-items:center;gap:12px;'>
        %1
        <div>
          <div class='clinic-name'>%2</div>
          <div class='clinic-sub'>%3</div>
          %4
        </div>
      </div>
      <div class='print-date'>
        Datum ispisa:<br/><b>%5</b>
      </div>
    </div>
    )").arg(logoHtml,
            escape(cs.clinicName),
            escape(cs.clinicSubtitle),
            contactLine,
            QDate::currentDate().toString("dd.MM.yyyy"));
}

static QString footerHtml()
{
    return R"(
    <div class='footer'>
      DentaPro — Sistem za upravljanje zubarskom ordinacijom | Povjerljivi medicinski dokument
    </div>
    )";
}

QString ReportGenerator::buildPatientReportHtml(const Patient& patient,
                                                 const QList<Intervention>& interventions)
{
    QString html = "<html><head>" + cssBase() + "</head><body>";
    html += headerHtml();

    // Patient info
    html += "<h2>👤 Podaci o pacijentu</h2>";
    html += "<div class='summary-box'><table>";
    auto info = [&](const QString& lbl, const QString& val) {
        if (!val.isEmpty())
            html += "<tr><td>" + escape(lbl) + ":</td><td><b>" + escape(val) + "</b></td></tr>";
    };
    info("Ime i prezime", patient.fullName());
    info("Datum rođenja", patient.dateOfBirth.isEmpty() ? "" :
         QDate::fromString(patient.dateOfBirth,"yyyy-MM-dd").toString("dd.MM.yyyy."));
    info("Spol", patient.gender);
    info("Telefon", patient.phone);
    info("E-mail", patient.email);
    info("Adresa", patient.address);
    info("Alergije", patient.allergies);
    info("Krvna grupa", patient.bloodType);
    html += "</table></div>";

    if (!patient.notes.isEmpty())
        html += "<p><b>Napomene:</b> " + escape(patient.notes) + "</p><br/>";

    // Summary stats
    double total = 0;
    for (const auto& iv : interventions) total += iv.cost;
    QString currency = interventions.isEmpty() ? "BAM" : interventions.first().currency;

    html += QString("<h2>📊 Sažetak</h2>"
                    "<div class='summary-box'><table>"
                    "<tr><td>Ukupno intervencija:</td><td><b>%1</b></td></tr>"
                    "<tr><td>Ukupno troškovi:</td><td><b>%2 %3</b></td></tr>"
                    "</table></div>")
            .arg(interventions.size())
            .arg(total, 0, 'f', 2)
            .arg(escape(currency));

    // Interventions
    html += "<h2>🦷 Historija intervencija</h2>";
    if (interventions.isEmpty()) {
        html += "<p style='color:#9E9E9E;'>Nema intervencija.</p>";
    } else {
        for (const auto& iv : interventions) {
            QString dt = iv.dateTime.length() >= 10 ?
                         QDate::fromString(iv.dateTime.left(10),"yyyy-MM-dd").toString("dd.MM.yyyy.") : iv.dateTime;
            if (iv.dateTime.length() >= 16)
                dt += " " + iv.dateTime.right(5);

            QString statusCls = "status-" + iv.status;
            QString statusLabel = iv.status == "completed" ? "Završeno" :
                                  iv.status == "scheduled" ? "Zakazano" : "Otkazano";

            html += "<div class='iv-card'>";
            html += "<div class='iv-header'>🦷 " + escape(iv.interventionType)
                 + " &nbsp;|&nbsp; " + escape(dt) + "</div>";
            html += "<div class='iv-body'><table>";

            auto ivrow = [&](const QString& l, const QString& v) {
                if (!v.isEmpty())
                    html += "<tr><td>" + escape(l) + ":</td><td>" + escape(v) + "</td></tr>";
            };
            ivrow("Zub", iv.tooth);
            ivrow("Doktor", iv.doctor);
            ivrow("Dijagnoza", iv.diagnosis);
            ivrow("Terapija", iv.treatment);
            ivrow("Opis rada", iv.description);
            ivrow("Napomene", iv.notes);
            html += QString("<tr><td>Cijena:</td><td><span class='cost-badge'>%1 %2</span></td></tr>")
                    .arg(iv.cost, 0, 'f', 2).arg(escape(iv.currency));
            html += QString("<tr><td>Status:</td><td><span class='%1'>%2</span></td></tr>")
                    .arg(statusCls, statusLabel);

            if (!iv.pdfPath.isEmpty())
                html += "<tr><td colspan='2'><div class='pdf-note'>📄 Priložen PDF dokument: "
                        + escape(iv.pdfPath) + "</div></td></tr>";
            if (!iv.signatureImagePath.isEmpty())
                html += "<tr><td colspan='2'><div class='sig-section'>"
                        "<b>✍️ Potpis pacijenta:</b><br/>"
                        "<img src='" + iv.signatureImagePath + "' style='max-width:220px; "
                        "border:1px solid #E0E0E0; border-radius:4px; margin-top:4px;'/>"
                        "</div></td></tr>";

            html += "</table></div></div>";
        }
    }

    html += footerHtml();
    html += "</body></html>";
    return html;
}

QString ReportGenerator::buildInterventionHtml(const Intervention& iv, const Patient& patient)
{
    QString html = "<html><head>" + cssBase() + "</head><body>";
    html += headerHtml();

    html += "<h2>🦷 Izvještaj o intervenciji</h2>";
    html += "<h3>Pacijent: " + escape(patient.fullName()) + "</h3>";

    QString dt = iv.dateTime.length() >= 16 ?
                 QDateTime::fromString(iv.dateTime,"yyyy-MM-dd HH:mm").toString("dd.MM.yyyy. HH:mm") :
                 iv.dateTime;

    html += "<div class='iv-card'>";
    html += "<div class='iv-header'>🦷 " + escape(iv.interventionType) + " — " + escape(dt) + "</div>";
    html += "<div class='iv-body'><table>";

    auto row = [&](const QString& l, const QString& v) {
        if (!v.isEmpty())
            html += "<tr><td>" + escape(l) + ":</td><td>" + escape(v) + "</td></tr>";
    };
    row("Pacijent", patient.fullName());
    row("Datum/Vrijeme", dt);
    row("Zub", iv.tooth);
    row("Doktor", iv.doctor);
    row("Dijagnoza", iv.diagnosis);
    row("Terapija", iv.treatment);
    row("Opis rada", iv.description);
    row("Napomene", iv.notes);
    html += QString("<tr><td>Cijena:</td><td><span class='cost-badge'>%1 %2</span></td></tr>")
            .arg(iv.cost, 0, 'f', 2).arg(escape(iv.currency));

    if (!iv.pdfPath.isEmpty())
        html += "<tr><td colspan='2'><div class='pdf-note'>📄 PDF: " + escape(iv.pdfPath) + "</div></td></tr>";

    // Signature
    html += "<tr><td colspan='2'><hr style='margin:12px 0; border-color:#E0E0E0;'/></td></tr>";
    if (!iv.signatureImagePath.isEmpty()) {
        html += "<tr><td colspan='2'><b>✍️ Potpis pacijenta:</b><br/>"
                "<img src='" + iv.signatureImagePath + "' style='max-width:250px; border:1px solid #ccc;'/>"
                "</td></tr>";
    } else {
        html += "<tr><td colspan='2'><b>✍️ Potpis pacijenta:</b>"
                "<div class='sig-line'></div>"
                "<div style='font-size:8pt; color:#757575;'>" + escape(patient.fullName()) + "</div>"
                "</td></tr>";
    }

    html += "</table></div></div>";
    html += footerHtml();
    html += "</body></html>";
    return html;
}

void ReportGenerator::printPatientReport(const Patient& patient,
                                          const QList<Intervention>& interventions,
                                          QWidget* parent)
{
    QString html = buildPatientReportHtml(patient, interventions);
    QTextDocument doc;
    doc.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15,15,15,15), QPageLayout::Millimeter);

    QPrintPreviewDialog preview(&printer, parent);
    preview.setWindowTitle("Pregled ispisa — " + patient.fullName());
    preview.resize(900, 700);

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                     [&](QPrinter* p) { doc.print(p); });
    preview.exec();
}

void ReportGenerator::printIntervention(const Intervention& iv,
                                         const Patient& patient,
                                         QWidget* parent)
{
    QString html = buildInterventionHtml(iv, patient);
    QTextDocument doc;
    doc.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15,15,15,15), QPageLayout::Millimeter);

    QPrintPreviewDialog preview(&printer, parent);
    preview.setWindowTitle("Ispis intervencije");
    preview.resize(900, 700);

    QObject::connect(&preview, &QPrintPreviewDialog::paintRequested,
                     [&](QPrinter* p) { doc.print(p); });
    preview.exec();
}
