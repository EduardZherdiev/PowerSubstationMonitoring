#include "substationdiagramview.h"

#include "diagramlinkitem.h"
#include "diagramnodeitem.h"
#include "diagramtheme.h"
#include "eventlogger.h"

#include <QBrush>
#include <QFont>
#include <QGraphicsScene>
#include <QLineF>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QFrame>
#include <QResizeEvent>
#include <QShowEvent>

SubstationDiagramView::SubstationDiagramView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_fitPending(true)
{
    setScene(m_scene);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignCenter);

}

void SubstationDiagramView::setLayout(const SubstationLayout::Layout &layout)
{
    m_layout = layout;
    buildDiagram();
}

const SubstationLayout::Layout &SubstationDiagramView::substationLayout() const
{
    return m_layout;
}

void SubstationDiagramView::fitToContent()
{
    logInfo("SubstationDiagramView", "Fitting diagram to content");
    if (!m_scene || viewport()->size().isEmpty()) {
        m_fitPending = true;
        return;
    }

    const QRectF contentRect = m_scene->itemsBoundingRect();
    if (contentRect.isNull() || contentRect.isEmpty()) {
        return;
    }

    const QRectF paddedRect = contentRect.adjusted(-12, -12, 12, 12);
    m_scene->setSceneRect(paddedRect);
    resetTransform();
    fitInView(paddedRect, Qt::KeepAspectRatio);
    m_fitPending = false;
}

void SubstationDiagramView::selectEquipment(const QString &equipmentKey)
{
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        it.value()->setSelectedAppearance(it.key() == equipmentKey);
    }

    bool selected;
    for (const ConnectionRecord &connection : std::as_const(m_connections)) {
        selected = connection.sourceKey == equipmentKey || connection.targetKey == equipmentKey;
        if (connection.item) {
            connection.item->setSelectedAppearance(selected);
        }
    }
}

void SubstationDiagramView::refreshTheme()
{
    if (!m_scene) {
        return;
    }

    m_scene->setBackgroundBrush(QBrush(DiagramTheme::color(DiagramTheme::ColorRole::Background)));
    viewport()->update();
    if (scene()) {
        scene()->update();
    }
    update();
}

DiagramNodeItem *SubstationDiagramView::addNode(const SubstationLayout::NodeSpec &nodeSpec)
{
    auto *node = new DiagramNodeItem(nodeSpec.id, nodeSpec.shape, nodeSpec.title, nodeSpec.size);
    node->setPos(nodeSpec.position);
    m_scene->addItem(node);
    connect(node, &DiagramNodeItem::activated, this, &SubstationDiagramView::equipmentActivated);
    m_nodes.insert(nodeSpec.id, node);
    return node;
}

void SubstationDiagramView::addConnection(const SubstationLayout::ConnectionSpec &connectionSpec,
                                          const QPointF &start,
                                          const QPointF &end)
{
    const QColor resolvedColor = DiagramTheme::color(connectionSpec.colorRole);
    auto *link = new DiagramLinkItem(connectionSpec.fromId, connectionSpec.toId, QLineF(start, end), resolvedColor, connectionSpec.width);
    m_scene->addItem(link);
    m_connections.append(ConnectionRecord{connectionSpec.fromId, connectionSpec.toId, link});
}

void SubstationDiagramView::drawBackground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawBackground(painter, rect);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(DiagramTheme::color(DiagramTheme::ColorRole::Grid), 1));

    const int left = static_cast<int>(rect.left()) - (static_cast<int>(rect.left()) % 40);
    const int top = static_cast<int>(rect.top()) - (static_cast<int>(rect.top()) % 40);

    for (int x = left; x < rect.right(); x += 40) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    for (int y = top; y < rect.bottom(); y += 40) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }

    painter->restore();
}

void SubstationDiagramView::resizeEvent(QResizeEvent *event)
{
    logInfo("SubstationDiagramView", "Resize event triggered");
    QGraphicsView::resizeEvent(event);
    fitToContent();
}

void SubstationDiagramView::buildDiagram()
{
    m_scene->clear();
    m_nodes.clear();
    m_connections.clear();

    m_scene->setBackgroundBrush(QBrush(DiagramTheme::color(DiagramTheme::ColorRole::Background)));

    for (const SubstationLayout::NodeSpec &nodeSpec : std::as_const(m_layout.nodes)) {
        addNode(nodeSpec);
    }

    DiagramNodeItem *sourceNode;
    DiagramNodeItem *targetNode;
    QPointF start;
    QPointF end;
    for (const SubstationLayout::ConnectionSpec &connectionSpec : std::as_const(m_layout.connections)) {
        targetNode = m_nodes.value(connectionSpec.toId, nullptr);
        sourceNode = m_nodes.value(connectionSpec.fromId, nullptr);
        if (!sourceNode || !targetNode) {
            continue;
        }

        start = anchorPoint(sourceNode, true);
        end = anchorPoint(targetNode, false);
        addConnection(connectionSpec, start, end);
    }

    const QRectF contentRect = m_scene->itemsBoundingRect();
    if (!contentRect.isNull() && !contentRect.isEmpty()) {
        m_scene->setSceneRect(contentRect.adjusted(-12, -12, 12, 12));
    } else {
        m_scene->setSceneRect(QRectF(0, 0, 1500, 520));
    }

    m_fitPending = true;
}

QPointF SubstationDiagramView::anchorPoint(const DiagramNodeItem *node, bool rightSide) const
{
    if (!node) {
        return QPointF();
    }

    const QRectF rect = node->boundingRect();
    const QPointF topLeft = node->pos();
    return rightSide ? QPointF(topLeft.x() + rect.right(), topLeft.y() + rect.center().y())
                     : QPointF(topLeft.x() + rect.left(), topLeft.y() + rect.center().y());
}
