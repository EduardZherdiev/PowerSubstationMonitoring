#ifndef DIAGRAMLINKITEM_H
#define DIAGRAMLINKITEM_H

#include <QColor>
#include <QGraphicsLineItem>
#include <QLineF>
#include <QString>

class QGraphicsItem;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class DiagramLinkItem : public QGraphicsLineItem {
  public:
    DiagramLinkItem(const QString& sourceKey, const QString& targetKey, const QLineF& line, const QColor& normalColor,
                    qreal normalWidth, QGraphicsItem* parent = nullptr);

    QString equipmentKey() const;
    QString sourceKey() const;
    QString targetKey() const;
    void setSelectedAppearance(bool selected);
    void refreshTheme();
    void setLine(const QLineF& line);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

  private:
    QString m_sourceKey;
    QString m_targetKey;
    QColor m_normalColor;
    qreal m_normalWidth;
    bool m_selected = false;
};

#endif // DIAGRAMLINKITEM_H
