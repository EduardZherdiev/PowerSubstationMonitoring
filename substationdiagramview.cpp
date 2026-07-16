#include "substationdiagramview.h"

#include "diagramlinkitem.h"
#include "diagramnodeitem.h"
#include "diagramtheme.h"
#include "eventlogger.h"

#include <QBrush>
#include <QFont>
#include <QFrame>
#include <QGraphicsScene>
#include <QLineF>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
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
    setDragMode(QGraphicsView::ScrollHandDrag);
}

void SubstationDiagramView::setLayout(const SubstationLayout::Layout &layout)
{
    m_layout = layout;
    buildDiagram();
    fitToContent();
}

const SubstationLayout::Layout &SubstationDiagramView::substationLayout() const
{
    return m_layout;
}

SubstationLayout::Layout SubstationDiagramView::layoutWithCurrentPositions() const
{
    SubstationLayout::Layout layout = m_layout;
    for (SubstationLayout::NodeSpec &node : layout.nodes) {
        if (const DiagramNodeItem *item = m_nodes.value(node.id, nullptr)) {
            node.position = item->pos();
        }
    }
    return layout;
}

void SubstationDiagramView::fitToContent()
{
    logInfo("SubstationDiagramView", tr("Fitting diagram to content"));
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
    for (const ConnectionRecord &connection : std::as_const(m_connections)) {
        if (connection.item) {
            connection.item->refreshTheme();
        }
    }
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
    connect(node,
            &DiagramNodeItem::positionChanged,
            this,
            &SubstationDiagramView::updateNodePosition);
    m_nodes.insert(nodeSpec.id, node);
    return node;
}

void SubstationDiagramView::addConnection(const SubstationLayout::ConnectionSpec &connectionSpec,
                                          DiagramNodeItem *sourceNode,
                                          DiagramNodeItem *targetNode)
{
    Q_UNUSED(connectionSpec.colorRole)
    Q_UNUSED(connectionSpec.width)
    const QColor resolvedColor = DiagramTheme::color(DiagramTheme::ColorRole::Branch);
    const QLineF line(anchorPoint(sourceNode, targetNode), anchorPoint(targetNode, sourceNode));
    auto *link = new DiagramLinkItem(connectionSpec.fromId,
                                     connectionSpec.toId,
                                     line,
                                     resolvedColor,
                                     2.0);
    m_scene->addItem(link);
    m_connections.append(
        ConnectionRecord{connectionSpec.fromId, connectionSpec.toId, sourceNode, targetNode, link});
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
    logInfo("SubstationDiagramView", tr("Resize event triggered"));
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
    for (const SubstationLayout::ConnectionSpec &connectionSpec :
         std::as_const(m_layout.connections)) {
        targetNode = m_nodes.value(connectionSpec.toId, nullptr);
        sourceNode = m_nodes.value(connectionSpec.fromId, nullptr);
        if (!sourceNode || !targetNode) {
            continue;
        }

        addConnection(connectionSpec, sourceNode, targetNode);
    }

    const QRectF contentRect = m_scene->itemsBoundingRect();
    if (!contentRect.isNull() && !contentRect.isEmpty()) {
        m_scene->setSceneRect(contentRect.adjusted(-12, -12, 12, 12));
    } else {
        m_scene->setSceneRect(QRectF(0, 0, 1500, 520));
    }

    m_fitPending = true;
}

QPointF SubstationDiagramView::anchorPoint(const DiagramNodeItem *node,
                                           const DiagramNodeItem *other) const
{
    if (!node || !other) {
        return QPointF();
    }

    const QRectF rect = node->boundingRect();
    const QPointF center = node->pos() + rect.center();
    const QPointF otherCenter = other->pos() + other->boundingRect().center();
    const QPointF direction = otherCenter - center;

    if (qFuzzyIsNull(direction.x()) && qFuzzyIsNull(direction.y())) {
        return center;
    }

    const qreal halfWidth = qMax<qreal>(1.0, rect.width() / 2.0);
    const qreal halfHeight = qMax<qreal>(1.0, rect.height() / 2.0);
    const qreal scale = 1.0
                        / qMax(qAbs(direction.x()) / halfWidth, qAbs(direction.y()) / halfHeight);
    return center + direction * scale;
}

void SubstationDiagramView::updateConnectionLines()
{
    for (const ConnectionRecord &connection : std::as_const(m_connections)) {
        if (connection.item && connection.sourceNode && connection.targetNode) {
            connection.item->setLine(
                QLineF(anchorPoint(connection.sourceNode, connection.targetNode),
                       anchorPoint(connection.targetNode, connection.sourceNode)));
        }
    }
}

void SubstationDiagramView::updateNodePosition(const QString &equipmentKey, const QPointF &position)
{
    for (SubstationLayout::NodeSpec &node : m_layout.nodes) {
        if (node.id == equipmentKey) {
            node.position = position;
            break;
        }
    }
    updateConnectionLines();
    const QRectF contentRect = m_scene->itemsBoundingRect();
    if (!contentRect.isEmpty()) {
        m_scene->setSceneRect(contentRect.adjusted(-24, -24, 24, 24));
    }
}
