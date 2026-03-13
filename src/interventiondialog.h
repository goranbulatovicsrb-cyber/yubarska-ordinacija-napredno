#pragma once
#include <QDialog>
#include "models.h"

class QLineEdit;
class QComboBox;
class QDateTimeEdit;
class QTextEdit;
class QLabel;
class QDoubleSpinBox;
class SignatureWidget;

class InterventionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InterventionDialog(QWidget* parent = nullptr,
                                 int patientId = 0,
                                 const Intervention& iv = {});
    Intervention getIntervention() const;

private slots:
    void accept() override;
    void browsePdf();
    void clearSignature();
    void browsePhotoBefore();
    void browsePhotoAfter();

private:
    void setupUi();
    void populateFields();
    void updatePhotoPreview(QLabel* lbl, const QString& path);

    Intervention     m_iv;
    int              m_patientId = 0;

    QComboBox*       m_type          = nullptr;
    QLineEdit*       m_tooth         = nullptr;
    QTextEdit*       m_diagnosis     = nullptr;
    QTextEdit*       m_treatment     = nullptr;
    QTextEdit*       m_description   = nullptr;
    QComboBox*       m_doctorCombo   = nullptr;
    QDateTimeEdit*   m_dateTime      = nullptr;
    QDoubleSpinBox*  m_cost          = nullptr;
    QComboBox*       m_currency      = nullptr;
    QComboBox*       m_status        = nullptr;
    QLineEdit*       m_pdfPath       = nullptr;
    QTextEdit*       m_notes         = nullptr;
    SignatureWidget* m_signature     = nullptr;

    // Photos
    QLabel*          m_photoBeforePrev = nullptr;
    QLabel*          m_photoAfterPrev  = nullptr;
    QString          m_photoBeforePath;
    QString          m_photoAfterPath;

    QString          m_savedSigPath;
};
