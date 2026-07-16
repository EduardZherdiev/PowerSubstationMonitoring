#include "powerflowcalculator.h"

#include <QHash>
#include <QQueue>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QtMath>

namespace {

QString normalize(const QString& value) {
    return value.trimmed().toLower();
}

QString formatNumber(double value, int precision = 2) {
    QString text = QString::number(value, 'f', precision);
    while (text.contains('.') && (text.endsWith('0') || text.endsWith('.'))) {
        if (text.endsWith('.')) {
            text.chop(1);
            break;
        }
        text.chop(1);
    }
    return text;
}

double parseFirstNumber(const QString& value, bool* ok = nullptr) {
    static const QRegularExpression numberPattern(QStringLiteral(R"(([+-]?\d+(?:\.\d+)?))"));
    const QRegularExpressionMatch match = numberPattern.match(value);
    if (!match.hasMatch()) {
        if (ok) {
            *ok = false;
        }
        return 0.0;
    }

    bool parsed = false;
    const double number = match.captured(1).toDouble(&parsed);
    if (ok) {
        *ok = parsed;
    }
    return parsed ? number : 0.0;
}

double parseVoltageKv(const QString& value, bool* ok = nullptr) {
    const QString normalized = normalize(value);
    bool parsed = false;
    const double number = parseFirstNumber(value, &parsed);
    if (!parsed) {
        if (ok) {
            *ok = false;
        }
        return 0.0;
    }

    if (normalized.contains(QStringLiteral("mv"))) {
        if (ok) {
            *ok = true;
        }
        return number * 1000.0;
    }

    if (normalized.contains(QStringLiteral("kv"))) {
        if (ok) {
            *ok = true;
        }
        return number;
    }

    if (normalized.contains(QStringLiteral("v"))) {
        if (ok) {
            *ok = true;
        }
        return number / 1000.0;
    }

    if (ok) {
        *ok = true;
    }
    return number;
}

double parseCurrentA(const QString& value, bool* ok = nullptr) {
    bool parsed = false;
    const double number = parseFirstNumber(value, &parsed);
    if (ok) {
        *ok = parsed;
    }
    return parsed ? number : 0.0;
}

double parseMva(const QString& value, bool* ok = nullptr) {
    bool parsed = false;
    const double number = parseFirstNumber(value, &parsed);
    if (ok) {
        *ok = parsed;
    }
    return parsed ? number : 0.0;
}

double parsePercent(const QString& value, bool* ok = nullptr) {
    bool parsed = false;
    const double number = parseFirstNumber(value, &parsed);
    if (ok) {
        *ok = parsed;
    }
    return parsed ? number : 0.0;
}

double parseTransformerRatioText(const QString& value, bool* ok = nullptr) {
    const QString normalized = normalize(value);
    const QStringList parts = normalized.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    if (parts.size() == 2) {
        bool numeratorOk = false;
        bool denominatorOk = false;
        const double numerator = parseFirstNumber(parts.at(0), &numeratorOk);
        const double denominator = parseFirstNumber(parts.at(1), &denominatorOk);
        if (numeratorOk && denominatorOk && numerator > 0.0 && denominator > 0.0) {
            if (ok) {
                *ok = true;
            }
            return numerator / denominator;
        }
    }

    bool parsed = false;
    const double number = parseFirstNumber(value, &parsed);
    if (ok) {
        *ok = parsed && number > 0.0;
    }
    return parsed ? number : 0.0;
}

double parseTemperatureC(const QString& value, bool* ok = nullptr) {
    bool parsed = false;
    const double number = parseFirstNumber(value, &parsed);
    if (ok) {
        *ok = parsed;
    }
    return parsed ? number : 0.0;
}

double parseTransformerRatio(const SubstationLayout::NodeSpec& node, bool* ok = nullptr) {
    bool parsed = false;
    double ratio = 0.0;
    QMap<QString, QString>::const_iterator it;

    const QStringList ratioKeys = {QStringLiteral("Transformation Ratio"), QStringLiteral("Transformation Coefficient"),
                                   QStringLiteral("Transformer Ratio"), QStringLiteral("Turns Ratio"),
                                   QStringLiteral("Ratio")};
    for (const QString& key : ratioKeys) {
        it = node.parameters.constFind(key);
        if (it != node.parameters.constEnd()) {
            ratio = parseTransformerRatioText(it.value(), &parsed);
            if (parsed && ratio > 0.0) {
                break;
            }
        }
    }

    if (!parsed || ratio <= 0.0) {
        double primaryKv = 0.0;
        double secondaryKv = 0.0;
        bool primaryOk = false;
        bool secondaryOk = false;

        it = node.parameters.constFind(QStringLiteral("Primary Voltage"));
        if (it != node.parameters.constEnd()) {
            primaryKv = parseVoltageKv(it.value(), &primaryOk);
        }

        it = node.parameters.constFind(QStringLiteral("Secondary Voltage"));
        if (it != node.parameters.constEnd()) {
            secondaryKv = parseVoltageKv(it.value(), &secondaryOk);
        }

        if (primaryOk && secondaryOk && secondaryKv > 0.0) {
            ratio = primaryKv / secondaryKv;
            parsed = ratio > 0.0;
        }
    }

    if (ok) {
        *ok = parsed && ratio > 0.0;
    }
    return ratio;
}

bool isBreakerClosed(const SubstationLayout::NodeSpec& node) {
    return normalize(node.status).contains(QStringLiteral("closed"));
}

QString elementKind(const SubstationLayout::NodeSpec& node) {
    const QString type = normalize(node.type);
    if (type.contains(QStringLiteral("breaker"))) {
        return QStringLiteral("breaker");
    }
    if (type.contains(QStringLiteral("transformer"))) {
        return QStringLiteral("transformer");
    }
    if (type.contains(QStringLiteral("busbar"))) {
        return QStringLiteral("busbar");
    }
    if (type.contains(QStringLiteral("line"))) {
        return QStringLiteral("line");
    }
    return QStringLiteral("device");
}

QString voltageText(const PowerFlowCalculator::NodeState& state) {
    return state.hasVoltage ? QStringLiteral("%1 kV").arg(formatNumber(state.voltageKv, 2)) : QStringLiteral("-");
}

QString currentText(const PowerFlowCalculator::NodeState& state) {
    return state.hasCurrent ? QStringLiteral("%1 A").arg(formatNumber(state.currentA, 2)) : QStringLiteral("-");
}

QString temperatureText(const PowerFlowCalculator::NodeState& state) {
    return state.hasTemperature ? QStringLiteral("%1 C").arg(formatNumber(state.temperatureC, 1)) : QStringLiteral("-");
}

PowerFlowCalculator::NodeState sourceStateFor(const SubstationLayout::NodeSpec& node) {
    PowerFlowCalculator::NodeState state;
    state.energized = true;

    bool voltageOk = false;
    bool currentOk = false;
    const auto voltageIt = node.parameters.constFind(QStringLiteral("Voltage"));
    const auto currentIt = node.parameters.constFind(QStringLiteral("Current"));
    if (voltageIt != node.parameters.constEnd()) {
        state.voltageKv = parseVoltageKv(voltageIt.value(), &voltageOk);
        state.hasVoltage = voltageOk;
    }
    if (currentIt != node.parameters.constEnd()) {
        state.currentA = parseCurrentA(currentIt.value(), &currentOk);
        state.hasCurrent = currentOk;
    }

    state.note = QStringLiteral("Source node");
    return state;
}

PowerFlowCalculator::NodeState propagateThroughBreaker(const PowerFlowCalculator::NodeState& incoming,
                                                       const SubstationLayout::NodeSpec& node) {
    if (!isBreakerClosed(node)) {
        PowerFlowCalculator::NodeState blocked;
        blocked.energized = false;
        blocked.upstreamId = incoming.upstreamId;
        blocked.note = QStringLiteral("Breaker open");
        return blocked;
    }

    PowerFlowCalculator::NodeState state = incoming;
    state.note = QStringLiteral("Breaker closed");
    return state;
}

PowerFlowCalculator::NodeState propagateThroughTransformer(const PowerFlowCalculator::NodeState& incoming,
                                                           const SubstationLayout::NodeSpec& node) {
    PowerFlowCalculator::NodeState state = incoming;
    state.note = QStringLiteral("Transformer");

    bool currentOk = false;
    bool loadOk = false;
    bool ratioOk = false;
    const double transformerRatio = parseTransformerRatio(node, &ratioOk);

    if (ratioOk && transformerRatio > 0.0 && incoming.hasVoltage) {
        state.voltageKv = incoming.voltageKv / transformerRatio;
        state.hasVoltage = true;
    }

    const auto mvaIt = node.parameters.constFind(QStringLiteral("MVA"));
    if (mvaIt != node.parameters.constEnd()) {
        const double ratedMva = parseMva(mvaIt.value(), &currentOk);
        double loadPercent = 100.0;
        const auto loadIt = node.parameters.constFind(QStringLiteral("Load"));
        if (loadIt != node.parameters.constEnd()) {
            loadPercent = parsePercent(loadIt.value(), &loadOk);
            if (!loadOk) {
                loadPercent = 100.0;
            }
        }

        if (currentOk && state.hasVoltage && state.voltageKv > 0.0) {
            const double apparentPowerMva = ratedMva * qMax(0.0, loadPercent) / 100.0;
            state.currentA = (apparentPowerMva * 1'000'000.0) / (qSqrt(3.0) * state.voltageKv * 1000.0);
            state.hasCurrent = true;
        }
    }

    if (ratioOk && transformerRatio > 0.0 && incoming.hasCurrent) {
        state.currentA = incoming.currentA * transformerRatio;
        state.hasCurrent = true;
    }

    if (!state.hasCurrent && incoming.hasCurrent) {
        state.currentA = incoming.currentA;
        state.hasCurrent = true;
    }

    const auto temperatureSensorIt = node.parameters.constFind(QStringLiteral("Temperature Sensor"));
    if (temperatureSensorIt != node.parameters.constEnd()) {
        const QString sensorId = temperatureSensorIt.value().trimmed();
        if (!sensorId.isEmpty()) {
            state.note = QStringLiteral("Temperature sensor: %1").arg(sensorId);
        }
    }

    return state;
}

PowerFlowCalculator::NodeState propagateNode(const SubstationLayout::NodeSpec& node,
                                             const PowerFlowCalculator::NodeState& incoming,
                                             const QMap<QString, double>& temperatureBySensor) {
    const QString kind = elementKind(node);
    if (kind == QStringLiteral("breaker")) {
        return propagateThroughBreaker(incoming, node);
    }

    if (kind == QStringLiteral("transformer")) {
        PowerFlowCalculator::NodeState state = propagateThroughTransformer(incoming, node);
        const auto temperatureSensorIt = node.parameters.constFind(QStringLiteral("Temperature Sensor"));
        if (temperatureSensorIt != node.parameters.constEnd()) {
            const QString sensorId = temperatureSensorIt.value().trimmed();
            const auto sensorIt = temperatureBySensor.constFind(sensorId);
            if (sensorIt != temperatureBySensor.constEnd()) {
                state.temperatureC = sensorIt.value();
                state.hasTemperature = true;
            }
        }

        if (!state.hasTemperature) {
            const auto directTemperatureIt = node.parameters.constFind(QStringLiteral("Temperature"));
            if (directTemperatureIt != node.parameters.constEnd()) {
                bool temperatureOk = false;
                state.temperatureC = parseTemperatureC(directTemperatureIt.value(), &temperatureOk);
                state.hasTemperature = temperatureOk;
            }
        }
        return state;
    }

    PowerFlowCalculator::NodeState state = incoming;
    state.note = kind == QStringLiteral("busbar") ? QStringLiteral("Busbar")
                 : kind == QStringLiteral("line") ? QStringLiteral("Line section")
                                                  : QStringLiteral("Pass-through");
    return state;
}

void annotateNode(SubstationLayout::NodeSpec* node, const PowerFlowCalculator::NodeState& state) {
    if (!node) {
        return;
    }

    bool ratioOk = false;
    const double ratio = parseTransformerRatio(*node, &ratioOk);

    node->parameters.insert(QStringLiteral("Calculated Voltage"), voltageText(state));
    node->parameters.insert(QStringLiteral("Calculated Current"), currentText(state));
    node->parameters.insert(QStringLiteral("Energized"),
                            state.energized ? QStringLiteral("Yes") : QStringLiteral("No"));
    node->parameters.insert(QStringLiteral("Upstream"),
                            state.upstreamId.isEmpty() ? QStringLiteral("-") : state.upstreamId);
    node->parameters.insert(QStringLiteral("Flow Note"), state.note.isEmpty() ? QStringLiteral("-") : state.note);

    if (state.hasTemperature) {
        node->parameters.insert(QStringLiteral("Temperature"), temperatureText(state));
    }
}

} // namespace

namespace PowerFlowCalculator {

QMap<QString, NodeState> calculate(const SubstationLayout::Layout& layout,
                                   const QMap<QString, double>& temperatureBySensor) {
    QMap<QString, NodeState> states;
    if (layout.nodes.isEmpty()) {
        return states;
    }

    QHash<QString, const SubstationLayout::NodeSpec*> nodeById;
    QHash<QString, QStringList> childrenById;
    QHash<QString, int> incomingCount;

    for (const SubstationLayout::NodeSpec& node : layout.nodes) {
        nodeById.insert(node.id, &node);
        incomingCount.insert(node.id, 0);
    }

    for (const SubstationLayout::ConnectionSpec& connection : layout.connections) {
        childrenById[connection.fromId].append(connection.toId);
        incomingCount[connection.toId] = incomingCount.value(connection.toId, 0) + 1;
    }

    QString sourceId = QStringLiteral("Line-1");
    if (!nodeById.contains(sourceId)) {
        for (const SubstationLayout::NodeSpec& node : layout.nodes) {
            if (incomingCount.value(node.id, 0) == 0) {
                sourceId = node.id;
                break;
            }
        }
    }

    if (!nodeById.contains(sourceId)) {
        return states;
    }

    QQueue<QString> queue;
    queue.enqueue(sourceId);
    states.insert(sourceId, sourceStateFor(*nodeById.value(sourceId)));

    QSet<QString> visited;
    visited.insert(sourceId);
    QString currentId;
    NodeState nextState;
    const SubstationLayout::NodeSpec* childNode;
    NodeState currentState;
    QStringList children;
    while (!queue.isEmpty()) {
        currentId = queue.dequeue();
        currentState = states.value(currentId);

        children = childrenById.value(currentId);
        for (const QString& childId : children) {
            if (!nodeById.contains(childId)) {
                continue;
            }

            childNode = nodeById.value(childId);
            nextState = propagateNode(*childNode, currentState, temperatureBySensor);
            nextState.upstreamId = currentId;
            if (currentState.energized && !nextState.energized) {
                nextState.note = QStringLiteral("Flow blocked by %1").arg(currentId);
            } else if (nextState.note.isEmpty()) {
                nextState.note = QStringLiteral("Inherited from %1").arg(currentId);
            }

            if (!states.contains(childId)) {
                states.insert(childId, nextState);
            }

            if (!visited.contains(childId)) {
                visited.insert(childId);
                if (nextState.energized) {
                    queue.enqueue(childId);
                }
            }
        }
    }

    return states;
}

void annotateLayout(SubstationLayout::Layout* layout, const QMap<QString, double>& temperatureBySensor) {
    if (!layout) {
        return;
    }

    const QMap<QString, NodeState> states = calculate(*layout, temperatureBySensor);
    auto stateIt = states.constBegin();
    for (SubstationLayout::NodeSpec& node : layout->nodes) {
        stateIt = states.constFind(node.id);
        if (stateIt != states.constEnd()) {
            annotateNode(&node, stateIt.value());
        }
    }
}

} // namespace PowerFlowCalculator
