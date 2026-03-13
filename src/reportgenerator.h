#pragma once
#include <QString>
#include "models.h"

class QPrinter;

class ReportGenerator
{
public:
    static void printPatientReport(const Patient& patient,
                                   const QList<Intervention>& interventions,
                                   QWidget* parent = nullptr);

    static void printIntervention(const Intervention& iv,
                                  const Patient& patient,
                                  QWidget* parent = nullptr);

    static QString buildPatientReportHtml(const Patient& patient,
                                          const QList<Intervention>& interventions);

    static QString buildInterventionHtml(const Intervention& iv,
                                         const Patient& patient);
};
