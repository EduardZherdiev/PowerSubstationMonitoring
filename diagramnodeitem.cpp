#include "diagramnodeitem.h"

#include "diagramtheme.h"

#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QVariant>

DiagramNodeItem::DiagramNodeItem(const QString &equipmentKey,
                                 ShapeType shapeType,
                                 const QString &title,
                                 const QSizeF &size,
                                 QGraphicsItem *parent)
    : QGraphicsObject(parent)
    , m_equipmentKey(equipmentKey)
    , m_shapeType(shapeType)
    , m_title(title)
    , m_size(size)
    , m_selected(false)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

QRectF DiagramNodeItem::boundingRect() const
{
    return QRectF(QPointF(0, 0), m_size);
}

void DiagramNodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    const QColor borderColor = m_selected ? DiagramTheme::color(DiagramTheme::ColorRole::Selection)
                                          : DiagramTheme::color(DiagramTheme::ColorRole::Border);
    const QColor fillColor = DiagramTheme::color(DiagramTheme::ColorRole::NodeFill);
    const QColor busColor = DiagramTheme::color(DiagramTheme::ColorRole::Busbar);

    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(QPen(borderColor, m_selected ? 3.5 : 2.0));
    painter->setBrush(Qt::NoBrush);

    const QRectF rect = boundingRect().adjusted(2, 2, -2, -2);

    switch (m_shapeType) {
    case ShapeType::Busbar:
        painter->setPen(QPen(busColor, 10.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawLine(QPointF(rect.left() + 8, rect.center().y()),
                          QPointF(rect.right() - 8, rect.center().y()));
        break;
    case ShapeType::Breaker:
        painter->setBrush(fillColor);
        painter->drawRect(QRectF(rect.center().x() - 18, rect.center().y() - 18, 36, 36));
        break;
    case ShapeType::Transformer: {
        painter->setBrush(Qt::NoBrush);
        QRectF leftCircle(rect.left() + 14, rect.center().y() - 24, 48, 48);
        QRectF rightCircle(rect.left() + 44, rect.center().y() - 24, 48, 48);
        painter->drawEllipse(leftCircle);
        painter->drawEllipse(rightCircle);
        break;
    }
    case ShapeType::LineTerminal:
        painter->setPen(QPen(DiagramTheme::color(DiagramTheme::ColorRole::Branch),
                             2.0,
                             Qt::SolidLine,
                             Qt::RoundCap,
                             Qt::RoundJoin));
        painter->drawLine(QPointF(rect.left() + 10, rect.center().y()),
                          QPointF(rect.right() - 10, rect.center().y()));
        {
            const QPointF leftTip(rect.left() + 18, rect.center().y());
            const QPointF rightTip(rect.right() - 18, rect.center().y());
            painter->setBrush(DiagramTheme::color(DiagramTheme::ColorRole::Branch));
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(leftTip, 5, 5);
            painter->drawEllipse(rightTip, 5, 5);
        }
        break;
    case ShapeType::Station:
        painter->setBrush(fillColor);
        painter->drawRoundedRect(rect.adjusted(10, 14, -10, -14), 12, 12);
        break;
    }

    painter->setPen(DiagramTheme::color(DiagramTheme::ColorRole::Text));
    painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
    painter->drawText(rect, Qt::AlignHCenter | Qt::AlignBottom, m_title);

    if (m_selected) {
        painter->setPen(
            QPen(DiagramTheme::color(DiagramTheme::ColorRole::Selection), 2.5, Qt::SolidLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect.adjusted(1, 1, -1, -1));
    }
}

QString DiagramNodeItem::equipmentKey() const
{
    return m_equipmentKey;
}

void DiagramNodeItem::setSelectedAppearance(bool selected)
{
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;
    update();
}

bool DiagramNodeItem::isSelectedAppearance() const
{
    return m_selected;
}

void DiagramNodeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    emit activated(m_equipmentKey);
    QGraphicsObject::mousePressEvent(event);
}

QVariant DiagramNodeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionHasChanged) {
        emit positionChanged(m_equipmentKey, value.toPointF());
    }
    return QGraphicsObject::itemChange(change, value);
}
