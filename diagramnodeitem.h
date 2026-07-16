#ifndef DIAGRAMNODEITEM_H
#define DIAGRAMNODEITEM_H

#include <QGraphicsObject>
#include <QMap>
#include <QPointF>
#include <QSizeF>
#include <QString>

class QGraphicsSceneMouseEvent;
class QGraphicsItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class QVariant;

class DiagramNodeItem : public QGraphicsObject {
    Q_OBJECT

  public:
    enum class ShapeType { Busbar, Breaker, Transformer, LineTerminal, Station };

    DiagramNodeItem(const QString& equipmentKey, ShapeType shapeType, const QString& title, const QSizeF& size,
                    QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    QString equipmentKey() const;
    void setSelectedAppearance(bool selected);
    bool isSelectedAppearance() const;

  signals:
    void activated(const QString& equipmentKey);
    void positionChanged(const QString& equipmentKey, const QPointF& position);

  protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  private:
    QString m_equipmentKey;
    ShapeType m_shapeType;
    QString m_title;
    QSizeF m_size;
    bool m_selected;
};

#endif // DIAGRAMNODEITEM_H
