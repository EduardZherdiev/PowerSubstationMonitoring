#include "diagramlinkitem.h"

#include "diagramtheme.h"

#include <QBrush>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <cmath>

DiagramLinkItem::DiagramLinkItem(const QString& equipmentKey, const QString& targetKey, const QLineF& line,
                                 const QColor& normalColor, qreal normalWidth, QGraphicsItem* parent)
    : QGraphicsLineItem(line, parent), m_sourceKey(equipmentKey), m_targetKey(targetKey), m_normalColor(normalColor),
      m_normalWidth(normalWidth) {
    setPen(QPen(m_normalColor, m_normalWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    setZValue(-1);
}

QString DiagramLinkItem::equipmentKey() const {
    return m_sourceKey;
}

QString DiagramLinkItem::sourceKey() const {
    return m_sourceKey;
}

QString DiagramLinkItem::targetKey() const {
    return m_targetKey;
}

void DiagramLinkItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    const QPen activePen = pen();
    const QColor dotColor = activePen.color();
    const qreal lineWidth = activePen.widthF();

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(activePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(line());

    const QPointF startPoint = line().p1();
    const QPointF endPoint = line().p2();
    const QPointF direction = endPoint - startPoint;
    const qreal length = std::hypot(direction.x(), direction.y());
    if (length <= 0.0) {
        return;
    }

    const QPointF unit(direction.x() / length, direction.y() / length);
    const QPointF normal(-unit.y(), unit.x());
    const qreal arrowSize = qMax<qreal>(8.0, lineWidth * 2.2);

    auto drawArrow = [&](qreal t) {
        const QPointF tip = startPoint + direction * t;
        QPainterPath arrow;
        arrow.moveTo(tip);
        arrow.lineTo(tip - unit * arrowSize + normal * (arrowSize * 0.55));
        arrow.lineTo(tip - unit * arrowSize - normal * (arrowSize * 0.55));
        arrow.closeSubpath();
        painter->setBrush(dotColor);
        painter->drawPath(arrow);
    };

    drawArrow(0.25);
    drawArrow(0.70);

    painter->setBrush(dotColor);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(startPoint, qMax<qreal>(3.0, lineWidth), qMax<qreal>(3.0, lineWidth));
    painter->drawEllipse(endPoint, qMax<qreal>(3.0, lineWidth), qMax<qreal>(3.0, lineWidth));
}

void DiagramLinkItem::setSelectedAppearance(bool selected) {
    m_selected = selected;
    if (selected) {
        setPen(QPen(DiagramTheme::color(DiagramTheme::ColorRole::Selection), m_normalWidth + 1.5, Qt::SolidLine,
                    Qt::RoundCap, Qt::RoundJoin));
    } else {
        setPen(QPen(m_normalColor, m_normalWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
}

void DiagramLinkItem::refreshTheme() {
    m_normalColor = DiagramTheme::color(DiagramTheme::ColorRole::Branch);
    setSelectedAppearance(m_selected);
    update();
}

void DiagramLinkItem::setLine(const QLineF& line) {
    QGraphicsLineItem::setLine(line);
    update();
}
