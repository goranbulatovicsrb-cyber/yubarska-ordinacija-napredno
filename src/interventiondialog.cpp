#include "interventiondialog.h"
#include "signaturewidget.h"
#include "clinicsettings.h"
#include <QLineEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QScrollArea>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QPixmap>
#include <QFile>
#include <QFrame>

InterventionDialog::InterventionDialog(QWidget* parent, int patientId, const Intervention& iv)
    : QDialog(parent), m_iv(iv), m_patientId(patientId)
{
    setWindowTitle(iv.id == 0 ? "Nova Intervencija" : "Uredi Intervenciju");
    setMinimumWidth(600);
    setMinimumHeight(700);
    setModal(true);
    setupUi();
    if (iv.id != 0) populateFields();
}

void InterventionDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    // ── Header ────────────────────────────────────────────────────────────────
    auto* header = new QWidget;
    header->setStyleSheet("background:#00695C; padding:16px;");
    auto* hLay = new QHBoxLayout(header);
    auto* title = new QLabel(m_iv.id == 0 ? "🦷  Nova Intervencija" : "🦷  Uredi Intervenciju");
    title->setStyleSheet("color:white; font-size:18px; font-weight:bold;");
    hLay->addWidget(title);
    mainLayout->addWidget(header);

    auto* scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);

    auto* formWidget = new QWidget;
    formWidget->setStyleSheet("background:#FAFAFA;");
    auto* outer = new QVBoxLayout(formWidget);
    outer->setContentsMargins(24,16,24,16);
    outer->setSpacing(16);

    auto fieldStyle = QString("QLineEdit,QDateTimeEdit,QComboBox,QDoubleSpinBox{"
                              "border:1px solid #BDBDBD;border-radius:4px;padding:4px 8px;"
                              "min-height:32px;}"
                              "QLineEdit:focus,QDateTimeEdit:focus,QComboBox:focus,QDoubleSpinBox:focus{"
                              "border:2px solid #00695C;}");
    auto textStyle = QString("QTextEdit{border:1px solid #BDBDBD;border-radius:4px;padding:4px;}"
                             "QTextEdit:focus{border:2px solid #00695C;}");
    auto lblStyle  = QString("font-weight:600; color:#424242;");

    auto makeGroup = [&](const QString& title) {
        auto* g = new QGroupBox(title);
        g->setStyleSheet("QGroupBox{font-weight:bold;color:#1B5E20;"
                         "border:1px solid #C8E6C9;border-radius:6px;margin-top:8px;"
                         "padding-top:8px;}"
                         "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 4px;}");
        return g;
    };

    // ── Section 1: Basic ─────────────────────────────────────────────────────
    auto* basicGroup = makeGroup("Osnovno");
    auto* basicForm  = new QFormLayout(basicGroup);
    basicForm->setSpacing(8);

    m_type = new QComboBox;
    m_type->setEditable(true);
    m_type->addItems({"Pregled","Vađenje zuba","Plomba","Devitalizacija","Protetika",
                      "Ortodoncija","Parodontologija","Beljenje","Rendgen","Čišćenje kamenca",
                      "Implant","Krunica","Most","Operacija","Konsultacija","Hitna pomoć"});
    m_type->setStyleSheet(fieldStyle);

    m_tooth = new QLineEdit;
    m_tooth->setPlaceholderText("npr. 16, 21, 36...");
    m_tooth->setStyleSheet(fieldStyle);

    // Doctor combo — populated from ClinicSettings
    m_doctorCombo = new QComboBox;
    m_doctorCombo->setEditable(true);
    auto& cs = ClinicSettings::instance();
    if (cs.doctors.isEmpty())
        m_doctorCombo->addItem("Dr.");
    else
        m_doctorCombo->addItems(cs.doctors);
    m_doctorCombo->setStyleSheet(fieldStyle);

    m_dateTime = new QDateTimeEdit(QDateTime::currentDateTime());
    m_dateTime->setCalendarPopup(true);
    m_dateTime->setDisplayFormat("dd.MM.yyyy HH:mm");
    m_dateTime->setStyleSheet(fieldStyle);

    m_status = new QComboBox;
    m_status->addItems({"completed","scheduled","cancelled"});
    m_status->setStyleSheet(fieldStyle);

    auto makeLbl = [&](const QString& t){
        auto* l = new QLabel(t); l->setStyleSheet(lblStyle); return l;
    };
    basicForm->addRow(makeLbl("Vrsta intervencije*:"), m_type);
    basicForm->addRow(makeLbl("Zub:"), m_tooth);
    basicForm->addRow(makeLbl("Doktor:"), m_doctorCombo);
    basicForm->addRow(makeLbl("Datum i vrijeme:"), m_dateTime);
    basicForm->addRow(makeLbl("Status:"), m_status);
    outer->addWidget(basicGroup);

    // ── Section 2: Medical notes ─────────────────────────────────────────────
    auto* medGroup = makeGroup("Medicinski podaci");
    auto* medForm  = new QFormLayout(medGroup);
    medForm->setSpacing(8);

    m_diagnosis = new QTextEdit;
    m_diagnosis->setPlaceholderText("Dijagnoza...");
    m_diagnosis->setFixedHeight(70);
    m_diagnosis->setStyleSheet(textStyle);

    m_treatment = new QTextEdit;
    m_treatment->setPlaceholderText("Terapija...");
    m_treatment->setFixedHeight(70);
    m_treatment->setStyleSheet(textStyle);

    m_description = new QTextEdit;
    m_description->setPlaceholderText("Detaljan opis rada...");
    m_description->setFixedHeight(80);
    m_description->setStyleSheet(textStyle);

    m_notes = new QTextEdit;
    m_notes->setPlaceholderText("Napomene...");
    m_notes->setFixedHeight(60);
    m_notes->setStyleSheet(textStyle);

    medForm->addRow(makeLbl("Dijagnoza:"), m_diagnosis);
    medForm->addRow(makeLbl("Terapija:"), m_treatment);
    medForm->addRow(makeLbl("Opis rada:"), m_description);
    medForm->addRow(makeLbl("Napomene:"), m_notes);
    outer->addWidget(medGroup);

    // ── Section 3: Financial ─────────────────────────────────────────────────
    auto* finGroup = makeGroup("Finansije");
    auto* finForm  = new QFormLayout(finGroup);
    finForm->setSpacing(8);

    m_cost = new QDoubleSpinBox;
    m_cost->setRange(0, 99999);
    m_cost->setDecimals(2);
    m_cost->setSingleStep(5.0);
    m_cost->setStyleSheet(fieldStyle);

    m_currency = new QComboBox;
    m_currency->addItems({"BAM","EUR","USD","RSD"});
    m_currency->setStyleSheet(fieldStyle);

    auto* costRow = new QHBoxLayout;
    costRow->addWidget(m_cost, 2);
    costRow->addWidget(m_currency, 1);

    finForm->addRow(makeLbl("Cijena:"), costRow);
    outer->addWidget(finGroup);

    // ── Section 4: PDF & Signature ───────────────────────────────────────────
    auto* docGroup = makeGroup("Dokument i Potpis");
    auto* docLay   = new QVBoxLayout(docGroup);
    docLay->setSpacing(8);

    auto* pdfRow = new QHBoxLayout;
    m_pdfPath = new QLineEdit;
    m_pdfPath->setPlaceholderText("Putanja do PDF dokumenta...");
    m_pdfPath->setReadOnly(true);
    m_pdfPath->setStyleSheet(fieldStyle);
    auto* browseBtn = new QPushButton("📎 Priloži PDF");
    browseBtn->setStyleSheet("QPushButton{background:#1565C0;color:white;border-radius:4px;"
                             "border:none;padding:6px 12px;font-weight:bold;}"
                             "QPushButton:hover{background:#1976D2;}");
    pdfRow->addWidget(m_pdfPath);
    pdfRow->addWidget(browseBtn);
    docLay->addLayout(pdfRow);

    auto* sigLbl = new QLabel("Elektronski potpis pacijenta:");
    sigLbl->setStyleSheet(lblStyle);
    docLay->addWidget(sigLbl);

    m_signature = new SignatureWidget;
    docLay->addWidget(m_signature);

    auto* sigBtnRow = new QHBoxLayout;
    sigBtnRow->addStretch();
    auto* clearSigBtn = new QPushButton("🗑 Obriši potpis");
    clearSigBtn->setStyleSheet("QPushButton{background:#EF5350;color:white;border-radius:4px;"
                               "border:none;padding:4px 10px;}"
                               "QPushButton:hover{background:#E53935;}");
    sigBtnRow->addWidget(clearSigBtn);
    docLay->addLayout(sigBtnRow);
    outer->addWidget(docGroup);

    // ── Section 5: Photos ────────────────────────────────────────────────────
    auto* photoGroup = makeGroup("Fotografije zuba (prije/poslije)");
    auto* photoLay   = new QHBoxLayout(photoGroup);
    photoLay->setSpacing(12);

    auto makePhotoBox = [&](const QString& label, QLabel*& preview,
                             const char* slot) -> QWidget* {
        auto* box = new QWidget;
        box->setStyleSheet("background:white;border:1px solid #E0E0E0;border-radius:6px;");
        auto* bl = new QVBoxLayout(box);
        bl->setContentsMargins(8, 8, 8, 8);
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet("font-weight:bold;color:#424242;font-size:10px;");
        preview = new QLabel;
        preview->setFixedSize(180, 120);
        preview->setAlignment(Qt::AlignCenter);
        preview->setStyleSheet(
            "border:1px dashed #BDBDBD;border-radius:4px;color:#9E9E9E;font-size:10px;");
        preview->setText("Nema fotografije");
        auto* btn = new QPushButton("Ucitaj fotografiju");
        btn->setStyleSheet(
            "QPushButton{background:#1565C0;color:white;border-radius:4px;"
            "border:none;padding:5px;font-size:10px;}"
            "QPushButton:hover{background:#1976D2;}");
        connect(btn, SIGNAL(clicked()), this, slot);
        bl->addWidget(lbl);
        bl->addWidget(preview);
        bl->addWidget(btn);
        return box;
    };

    photoLay->addWidget(makePhotoBox("PRIJE zahvata",
        m_photoBeforePrev, SLOT(browsePhotoBefore())));
    photoLay->addWidget(makePhotoBox("POSLIJE zahvata",
        m_photoAfterPrev,  SLOT(browsePhotoAfter())));
    outer->addWidget(photoGroup);

    scroll->setWidget(formWidget);
    mainLayout->addWidget(scroll, 1);

    // ── Buttons ────────────────────────────────────────────────────────────────
    auto* btnBar = new QWidget;
    btnBar->setStyleSheet("background:#EEEEEE; border-top:1px solid #E0E0E0;");
    auto* btnLay = new QHBoxLayout(btnBar);
    btnLay->setContentsMargins(24,12,24,12);
    btnLay->addStretch();

    auto* cancelBtn = new QPushButton("Odustani");
    cancelBtn->setMinimumSize(100,36);
    cancelBtn->setStyleSheet("QPushButton{border:1px solid #BDBDBD;border-radius:4px;"
                             "background:white;color:#424242;font-weight:bold;}"
                             "QPushButton:hover{background:#F5F5F5;}");

    auto* saveBtn = new QPushButton(m_iv.id == 0 ? "Dodaj Intervenciju" : "Sačuvaj");
    saveBtn->setMinimumSize(150,36);
    saveBtn->setStyleSheet("QPushButton{background:#00695C;color:white;border-radius:4px;"
                           "border:none;font-weight:bold;}"
                           "QPushButton:hover{background:#00897B;}");

    btnLay->addWidget(cancelBtn);
    btnLay->addWidget(saveBtn);
    mainLayout->addWidget(btnBar);

    connect(browseBtn,   &QPushButton::clicked, this, &InterventionDialog::browsePdf);
    connect(clearSigBtn, &QPushButton::clicked, this, &InterventionDialog::clearSignature);
    connect(cancelBtn,   &QPushButton::clicked, this, &QDialog::reject);
    connect(saveBtn,     &QPushButton::clicked, this, &InterventionDialog::accept);
}

void InterventionDialog::populateFields()
{
    int ti = m_type->findText(m_iv.interventionType);
    if (ti >= 0) m_type->setCurrentIndex(ti);
    else m_type->setEditText(m_iv.interventionType);
    m_tooth->setText(m_iv.tooth);

    // Set doctor combo
    int di = m_doctorCombo->findText(m_iv.doctor);
    if (di >= 0) m_doctorCombo->setCurrentIndex(di);
    else m_doctorCombo->setEditText(m_iv.doctor);

    if (!m_iv.dateTime.isEmpty())
        m_dateTime->setDateTime(QDateTime::fromString(m_iv.dateTime,"yyyy-MM-dd HH:mm"));
    int si = m_status->findText(m_iv.status);
    if (si >= 0) m_status->setCurrentIndex(si);
    m_diagnosis->setPlainText(m_iv.diagnosis);
    m_treatment->setPlainText(m_iv.treatment);
    m_description->setPlainText(m_iv.description);
    m_notes->setPlainText(m_iv.notes);
    m_cost->setValue(m_iv.cost);
    int ci = m_currency->findText(m_iv.currency);
    if (ci >= 0) m_currency->setCurrentIndex(ci);
    m_pdfPath->setText(m_iv.pdfPath);
    if (!m_iv.signatureImagePath.isEmpty())
        m_signature->loadFromFile(m_iv.signatureImagePath);
    m_savedSigPath = m_iv.signatureImagePath;

    // Load photos
    m_photoBeforePath = m_iv.photoBefore;
    m_photoAfterPath  = m_iv.photoAfter;
    updatePhotoPreview(m_photoBeforePrev, m_photoBeforePath);
    updatePhotoPreview(m_photoAfterPrev,  m_photoAfterPath);
}

void InterventionDialog::browsePdf()
{
    QString path = QFileDialog::getOpenFileName(this, "Priloži PDF dokument",
                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                   "PDF Dokumenti (*.pdf)");
    if (!path.isEmpty()) m_pdfPath->setText(path);
}

void InterventionDialog::clearSignature()
{
    m_signature->clear();
    m_savedSigPath.clear();
}

void InterventionDialog::browsePhotoBefore()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Fotografija PRIJE zahvata",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Slike (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;

    // Copy to AppData/photos
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/photos";
    QDir().mkpath(dir);
    QFileInfo fi(path);
    QString dest = dir + "/before_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")
                   + "." + fi.suffix();
    QFile::copy(path, dest);
    m_photoBeforePath = dest;
    updatePhotoPreview(m_photoBeforePrev, m_photoBeforePath);
}

void InterventionDialog::browsePhotoAfter()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Fotografija POSLIJE zahvata",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Slike (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/photos";
    QDir().mkpath(dir);
    QFileInfo fi(path);
    QString dest = dir + "/after_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")
                   + "." + fi.suffix();
    QFile::copy(path, dest);
    m_photoAfterPath = dest;
    updatePhotoPreview(m_photoAfterPrev, m_photoAfterPath);
}

void InterventionDialog::updatePhotoPreview(QLabel* lbl, const QString& path)
{
    if (path.isEmpty() || !QFile::exists(path)) {
        lbl->setPixmap(QPixmap());
        lbl->setText("Nema fotografije");
        return;
    }
    QPixmap px(path);
    if (!px.isNull()) {
        lbl->setPixmap(px.scaled(180, 120,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
        lbl->setText("");
    }
}

void InterventionDialog::accept()
{
    if (m_type->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Greska", "Vrsta intervencije je obavezna.");
        return;
    }
    m_iv.patientId        = m_patientId;
    m_iv.interventionType = m_type->currentText().trimmed();
    m_iv.tooth            = m_tooth->text().trimmed();
    m_iv.doctor           = m_doctorCombo->currentText().trimmed();
    m_iv.dateTime         = m_dateTime->dateTime().toString("yyyy-MM-dd HH:mm");
    m_iv.status           = m_status->currentText();
    m_iv.diagnosis        = m_diagnosis->toPlainText().trimmed();
    m_iv.treatment        = m_treatment->toPlainText().trimmed();
    m_iv.description      = m_description->toPlainText().trimmed();
    m_iv.notes            = m_notes->toPlainText().trimmed();
    m_iv.cost             = m_cost->value();
    m_iv.currency         = m_currency->currentText();
    m_iv.pdfPath          = m_pdfPath->text().trimmed();
    m_iv.photoBefore      = m_photoBeforePath;
    m_iv.photoAfter       = m_photoAfterPath;

    if (!m_signature->isEmpty()) {
        QString sigDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + "/signatures";
        QDir().mkpath(sigDir);
        QString sigPath = sigDir + "/sig_"
                          + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")
                          + ".png";
        if (m_signature->saveToFile(sigPath))
            m_iv.signatureImagePath = sigPath;
    } else {
        m_iv.signatureImagePath = m_savedSigPath;
    }

    QDialog::accept();
}

Intervention InterventionDialog::getIntervention() const { return m_iv; }
