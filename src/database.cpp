#include "database.h"
#include <QStandardPaths>
#include <QDir>

Database& Database::instance()
{
    static Database db;
    return db;
}

bool Database::initialize(const QString& dbPath)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    QString path = dbPath;
    if (path.isEmpty()) {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        path = dir + "/dentapro.db";
    }
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        qCritical() << "DB open failed:" << m_db.lastError().text();
        return false;
    }
    return createTables();
}

bool Database::createTables()
{
    QSqlQuery q;
    // Patients
    q.exec("PRAGMA journal_mode=WAL;");
    q.exec("PRAGMA foreign_keys=ON;");

    bool ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS patients (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            firstName   TEXT NOT NULL,
            lastName    TEXT NOT NULL,
            dateOfBirth TEXT,
            gender      TEXT,
            phone       TEXT,
            email       TEXT,
            address     TEXT,
            allergies   TEXT,
            bloodType   TEXT,
            notes       TEXT,
            createdAt   TEXT DEFAULT (datetime('now'))
        )
    )");
    if (!ok) { qCritical() << q.lastError(); return false; }

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS interventions (
            id                 INTEGER PRIMARY KEY AUTOINCREMENT,
            patientId          INTEGER NOT NULL REFERENCES patients(id) ON DELETE CASCADE,
            interventionType   TEXT,
            tooth              TEXT,
            diagnosis          TEXT,
            treatment          TEXT,
            description        TEXT,
            doctor             TEXT,
            dateTime           TEXT,
            pdfPath            TEXT,
            signatureImagePath TEXT,
            photoBefore        TEXT,
            photoAfter         TEXT,
            cost               REAL DEFAULT 0,
            currency           TEXT DEFAULT 'BAM',
            status             TEXT DEFAULT 'completed',
            notes              TEXT
        )
    )");
    if (!ok) { qCritical() << q.lastError(); return false; }

    // Migrations for existing databases
    q.exec("ALTER TABLE interventions ADD COLUMN photoBefore TEXT");
    q.exec("ALTER TABLE interventions ADD COLUMN photoAfter  TEXT");

    ok = q.exec(R"(
        CREATE TABLE IF NOT EXISTS appointments (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            patientId INTEGER NOT NULL REFERENCES patients(id) ON DELETE CASCADE,
            dateTime  TEXT,
            reason    TEXT,
            status    TEXT DEFAULT 'scheduled',
            notes     TEXT
        )
    )");
    if (!ok) { qCritical() << q.lastError(); return false; }

    // Indexes
    q.exec("CREATE INDEX IF NOT EXISTS idx_iv_patient ON interventions(patientId)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_iv_date    ON interventions(dateTime)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_ap_patient ON appointments(patientId)");

    return true;
}

// ─────────────────────── helpers ────────────────────────────────────────────

Patient Database::patientFromQuery(QSqlQuery& q)
{
    Patient p;
    p.id          = q.value("id").toInt();
    p.firstName   = q.value("firstName").toString();
    p.lastName    = q.value("lastName").toString();
    p.dateOfBirth = q.value("dateOfBirth").toString();
    p.gender      = q.value("gender").toString();
    p.phone       = q.value("phone").toString();
    p.email       = q.value("email").toString();
    p.address     = q.value("address").toString();
    p.allergies   = q.value("allergies").toString();
    p.bloodType   = q.value("bloodType").toString();
    p.notes       = q.value("notes").toString();
    p.createdAt   = q.value("createdAt").toString();
    return p;
}

Intervention Database::interventionFromQuery(QSqlQuery& q)
{
    Intervention iv;
    iv.id                = q.value("id").toInt();
    iv.patientId         = q.value("patientId").toInt();
    iv.interventionType  = q.value("interventionType").toString();
    iv.tooth             = q.value("tooth").toString();
    iv.diagnosis         = q.value("diagnosis").toString();
    iv.treatment         = q.value("treatment").toString();
    iv.description       = q.value("description").toString();
    iv.doctor            = q.value("doctor").toString();
    iv.dateTime          = q.value("dateTime").toString();
    iv.pdfPath           = q.value("pdfPath").toString();
    iv.signatureImagePath= q.value("signatureImagePath").toString();
    iv.photoBefore       = q.value("photoBefore").toString();
    iv.photoAfter        = q.value("photoAfter").toString();
    iv.cost              = q.value("cost").toDouble();
    iv.currency          = q.value("currency").toString();
    iv.status            = q.value("status").toString();
    iv.notes             = q.value("notes").toString();
    return iv;
}

Appointment Database::appointmentFromQuery(QSqlQuery& q)
{
    Appointment a;
    a.id        = q.value("id").toInt();
    a.patientId = q.value("patientId").toInt();
    a.dateTime  = q.value("dateTime").toString();
    a.reason    = q.value("reason").toString();
    a.status    = q.value("status").toString();
    a.notes     = q.value("notes").toString();
    return a;
}

// ─────────────────────── Patients ───────────────────────────────────────────

bool Database::addPatient(Patient& patient)
{
    QSqlQuery q;
    q.prepare(R"(INSERT INTO patients
        (firstName,lastName,dateOfBirth,gender,phone,email,address,allergies,bloodType,notes)
        VALUES (?,?,?,?,?,?,?,?,?,?))");
    q.addBindValue(patient.firstName);
    q.addBindValue(patient.lastName);
    q.addBindValue(patient.dateOfBirth);
    q.addBindValue(patient.gender);
    q.addBindValue(patient.phone);
    q.addBindValue(patient.email);
    q.addBindValue(patient.address);
    q.addBindValue(patient.allergies);
    q.addBindValue(patient.bloodType);
    q.addBindValue(patient.notes);
    if (!q.exec()) { qWarning() << q.lastError(); return false; }
    patient.id = q.lastInsertId().toInt();
    return true;
}

bool Database::updatePatient(const Patient& p)
{
    QSqlQuery q;
    q.prepare(R"(UPDATE patients SET
        firstName=?,lastName=?,dateOfBirth=?,gender=?,phone=?,
        email=?,address=?,allergies=?,bloodType=?,notes=?
        WHERE id=?)");
    q.addBindValue(p.firstName); q.addBindValue(p.lastName);
    q.addBindValue(p.dateOfBirth); q.addBindValue(p.gender);
    q.addBindValue(p.phone); q.addBindValue(p.email);
    q.addBindValue(p.address); q.addBindValue(p.allergies);
    q.addBindValue(p.bloodType); q.addBindValue(p.notes);
    q.addBindValue(p.id);
    if (!q.exec()) { qWarning() << q.lastError(); return false; }
    return true;
}

bool Database::deletePatient(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM patients WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}

QList<Patient> Database::getAllPatients()
{
    QList<Patient> list;
    QSqlQuery q("SELECT * FROM patients ORDER BY lastName, firstName");
    while (q.next()) list.append(patientFromQuery(q));
    return list;
}

QList<Patient> Database::searchPatients(const QString& query)
{
    QList<Patient> list;
    QSqlQuery q;
    QString pat = "%" + query + "%";
    q.prepare(R"(SELECT * FROM patients WHERE
        firstName LIKE ? OR lastName LIKE ? OR phone LIKE ? OR email LIKE ?
        ORDER BY lastName, firstName)");
    q.addBindValue(pat); q.addBindValue(pat);
    q.addBindValue(pat); q.addBindValue(pat);
    q.exec();
    while (q.next()) list.append(patientFromQuery(q));
    return list;
}

Patient Database::getPatient(int id)
{
    QSqlQuery q;
    q.prepare("SELECT * FROM patients WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) return patientFromQuery(q);
    return {};
}

// ─────────────────────── Interventions ──────────────────────────────────────

bool Database::addIntervention(Intervention& iv)
{
    QSqlQuery q;
    q.prepare(R"(INSERT INTO interventions
        (patientId,interventionType,tooth,diagnosis,treatment,description,
         doctor,dateTime,pdfPath,signatureImagePath,photoBefore,photoAfter,
         cost,currency,status,notes)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?))");
    q.addBindValue(iv.patientId);
    q.addBindValue(iv.interventionType);
    q.addBindValue(iv.tooth);
    q.addBindValue(iv.diagnosis);
    q.addBindValue(iv.treatment);
    q.addBindValue(iv.description);
    q.addBindValue(iv.doctor);
    q.addBindValue(iv.dateTime);
    q.addBindValue(iv.pdfPath);
    q.addBindValue(iv.signatureImagePath);
    q.addBindValue(iv.photoBefore);
    q.addBindValue(iv.photoAfter);
    q.addBindValue(iv.cost);
    q.addBindValue(iv.currency);
    q.addBindValue(iv.status);
    q.addBindValue(iv.notes);
    if (!q.exec()) { qWarning() << q.lastError(); return false; }
    iv.id = q.lastInsertId().toInt();
    return true;
}

bool Database::updateIntervention(const Intervention& iv)
{
    QSqlQuery q;
    q.prepare(R"(UPDATE interventions SET
        interventionType=?,tooth=?,diagnosis=?,treatment=?,description=?,
        doctor=?,dateTime=?,pdfPath=?,signatureImagePath=?,
        photoBefore=?,photoAfter=?,
        cost=?,currency=?,status=?,notes=?
        WHERE id=?)");
    q.addBindValue(iv.interventionType); q.addBindValue(iv.tooth);
    q.addBindValue(iv.diagnosis); q.addBindValue(iv.treatment);
    q.addBindValue(iv.description); q.addBindValue(iv.doctor);
    q.addBindValue(iv.dateTime); q.addBindValue(iv.pdfPath);
    q.addBindValue(iv.signatureImagePath);
    q.addBindValue(iv.photoBefore); q.addBindValue(iv.photoAfter);
    q.addBindValue(iv.cost); q.addBindValue(iv.currency);
    q.addBindValue(iv.status); q.addBindValue(iv.notes);
    q.addBindValue(iv.id);
    if (!q.exec()) { qWarning() << q.lastError(); return false; }
    return true;
}

bool Database::deleteIntervention(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM interventions WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}

QList<Intervention> Database::getPatientInterventions(int patientId)
{
    QList<Intervention> list;
    QSqlQuery q;
    q.prepare("SELECT * FROM interventions WHERE patientId=? ORDER BY dateTime DESC");
    q.addBindValue(patientId);
    q.exec();
    while (q.next()) list.append(interventionFromQuery(q));
    return list;
}

QList<Intervention> Database::getInterventionsByDate(int patientId, const QDate& date)
{
    QList<Intervention> list;
    QSqlQuery q;
    q.prepare("SELECT * FROM interventions WHERE patientId=? AND date(dateTime)=? ORDER BY dateTime");
    q.addBindValue(patientId);
    q.addBindValue(date.toString("yyyy-MM-dd"));
    q.exec();
    while (q.next()) list.append(interventionFromQuery(q));
    return list;
}

QList<QDate> Database::getInterventionDates(int patientId)
{
    QList<QDate> dates;
    QSqlQuery q;
    q.prepare("SELECT DISTINCT date(dateTime) AS d FROM interventions WHERE patientId=?");
    q.addBindValue(patientId);
    q.exec();
    while (q.next()) dates.append(QDate::fromString(q.value("d").toString(), "yyyy-MM-dd"));
    return dates;
}

Intervention Database::getIntervention(int id)
{
    QSqlQuery q;
    q.prepare("SELECT * FROM interventions WHERE id=?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) return interventionFromQuery(q);
    return {};
}

QList<Intervention> Database::searchInterventions(const QString& patientName,
                                                    const QDate& from,
                                                    const QDate& to,
                                                    const QString& type)
{
    QList<Intervention> list;
    QString sql = R"(
        SELECT i.* FROM interventions i
        JOIN patients p ON p.id = i.patientId
        WHERE 1=1
    )";
    if (!patientName.isEmpty())
        sql += " AND (p.firstName||' '||p.lastName) LIKE '%" + patientName + "%'";
    if (from.isValid())
        sql += " AND date(i.dateTime) >= '" + from.toString("yyyy-MM-dd") + "'";
    if (to.isValid())
        sql += " AND date(i.dateTime) <= '" + to.toString("yyyy-MM-dd") + "'";
    if (!type.isEmpty())
        sql += " AND i.interventionType LIKE '%" + type + "%'";
    sql += " ORDER BY i.dateTime DESC";
    QSqlQuery q(sql);
    while (q.next()) list.append(interventionFromQuery(q));
    return list;
}

// ─────────────────────── Appointments ───────────────────────────────────────

bool Database::addAppointment(Appointment& ap)
{
    QSqlQuery q;
    q.prepare("INSERT INTO appointments (patientId,dateTime,reason,status,notes) VALUES (?,?,?,?,?)");
    q.addBindValue(ap.patientId); q.addBindValue(ap.dateTime);
    q.addBindValue(ap.reason); q.addBindValue(ap.status); q.addBindValue(ap.notes);
    if (!q.exec()) { qWarning() << q.lastError(); return false; }
    ap.id = q.lastInsertId().toInt();
    return true;
}

bool Database::updateAppointment(const Appointment& ap)
{
    QSqlQuery q;
    q.prepare("UPDATE appointments SET dateTime=?,reason=?,status=?,notes=? WHERE id=?");
    q.addBindValue(ap.dateTime); q.addBindValue(ap.reason);
    q.addBindValue(ap.status); q.addBindValue(ap.notes); q.addBindValue(ap.id);
    return q.exec();
}

bool Database::deleteAppointment(int id)
{
    QSqlQuery q;
    q.prepare("DELETE FROM appointments WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}

QList<Appointment> Database::getUpcomingAppointments(int days)
{
    QList<Appointment> list;
    QString now = QDate::currentDate().toString("yyyy-MM-dd");
    QString future = QDate::currentDate().addDays(days).toString("yyyy-MM-dd");
    QSqlQuery q;
    q.prepare(R"(SELECT * FROM appointments WHERE date(dateTime) BETWEEN ? AND ?
                 AND status='scheduled' ORDER BY dateTime)");
    q.addBindValue(now); q.addBindValue(future);
    q.exec();
    while (q.next()) list.append(appointmentFromQuery(q));
    return list;
}

QList<Appointment> Database::getPatientAppointments(int patientId)
{
    QList<Appointment> list;
    QSqlQuery q;
    q.prepare("SELECT * FROM appointments WHERE patientId=? ORDER BY dateTime DESC");
    q.addBindValue(patientId);
    q.exec();
    while (q.next()) list.append(appointmentFromQuery(q));
    return list;
}

// ─────────────────────── Stats ───────────────────────────────────────────────

int Database::totalPatients()
{
    QSqlQuery q("SELECT COUNT(*) FROM patients");
    if (q.next()) return q.value(0).toInt();
    return 0;
}

int Database::totalInterventionsThisMonth()
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM interventions WHERE strftime('%Y-%m',dateTime)=?");
    q.addBindValue(QDate::currentDate().toString("yyyy-MM"));
    q.exec();
    if (q.next()) return q.value(0).toInt();
    return 0;
}

double Database::totalRevenueThisMonth()
{
    QSqlQuery q;
    q.prepare("SELECT COALESCE(SUM(cost),0) FROM interventions WHERE strftime('%Y-%m',dateTime)=?");
    q.addBindValue(QDate::currentDate().toString("yyyy-MM"));
    q.exec();
    if (q.next()) return q.value(0).toDouble();
    return 0.0;
}

QList<QPair<QString,double>> Database::revenueByMonth(int year)
{
    QList<QPair<QString,double>> result;
    static const QStringList months = {
        "Jan","Feb","Mar","Apr","Maj","Jun",
        "Jul","Aug","Sep","Okt","Nov","Dec"
    };
    for (int m = 1; m <= 12; ++m) {
        QSqlQuery q;
        q.prepare("SELECT COALESCE(SUM(cost),0) FROM interventions "
                  "WHERE strftime('%Y-%m',dateTime)=? AND status='completed'");
        q.addBindValue(QString("%1-%2").arg(year).arg(m,2,10,QChar('0')));
        q.exec();
        double val = q.next() ? q.value(0).toDouble() : 0.0;
        result.append({months[m-1], val});
    }
    return result;
}

QList<QPair<QString,int>> Database::interventionsByType(int year)
{
    QList<QPair<QString,int>> result;
    QSqlQuery q;
    q.prepare("SELECT intervention_type, COUNT(*) FROM interventions "
              "WHERE strftime('%Y',dateTime)=? "
              "GROUP BY intervention_type ORDER BY COUNT(*) DESC LIMIT 8");
    q.addBindValue(QString::number(year));
    if (q.exec())
        while (q.next())
            result.append({q.value(0).toString(), q.value(1).toInt()});
    return result;
}

QList<Appointment> Database::getTodayAppointments()
{
    QList<Appointment> list;
    QSqlQuery q;
    q.prepare("SELECT * FROM appointments WHERE date(dateTime)=date('now') "
              "AND status='scheduled' ORDER BY dateTime");
    q.exec();
    while (q.next()) list.append(appointmentFromQuery(q));
    return list;
}

QList<Appointment> Database::getAppointmentsInDays(int days)
{
    QList<Appointment> list;
    QSqlQuery q;
    q.prepare("SELECT * FROM appointments "
              "WHERE date(dateTime) BETWEEN date('now') AND date('now','+"
              + QString::number(days) + " days') "
              "AND status='scheduled' ORDER BY dateTime");
    q.exec();
    while (q.next()) list.append(appointmentFromQuery(q));
    return list;
}
