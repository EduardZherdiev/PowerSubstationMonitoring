#ifndef POWERFLOWCALCULATOR_H
#define POWERFLOWCALCULATOR_H

#include "substationlayout.h"

#include <QMap>
#include <QString>

namespace PowerFlowCalculator {

struct NodeState
{
    bool energized = false;
    bool hasVoltage = false;
    bool hasCurrent = false;
    double voltageKv = 0.0;
    double currentA = 0.0;
    double temperatureC = 0.0;
    bool hasTemperature = false;
    QString upstreamId;
    QString note;
};

QMap<QString, NodeState> calculate(const SubstationLayout::Layout &layout,
                                   const QMap<QString, double> &temperatureBySensor = QMap<QString, double>());
void annotateLayout(SubstationLayout::Layout *layout,
                    const QMap<QString, double> &temperatureBySensor = QMap<QString, double>());

} // namespace PowerFlowCalculator

#endif // POWERFLOWCALCULATOR_H