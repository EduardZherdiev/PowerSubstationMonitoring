#include "equipmenttreemodel.h"

#include "equipment.h"
#include "substationlayout.h"

#include <QCoreApplication>

namespace {

QString translatedType(const QString &type)
{
    if (type == QStringLiteral("Transmission Line")) {
        return QCoreApplication::translate("Equipment", "Transmission Line");
    }
    if (type == QStringLiteral("Circuit Breaker")) {
        return QCoreApplication::translate("Equipment", "Circuit Breaker");
    }
    if (type == QStringLiteral("Transformer")) {
        return QCoreApplication::translate("Equipment", "Transformer");
    }
    if (type == QStringLiteral("Busbar")) {
        return QCoreApplication::translate("Equipment", "Busbar");
    }
    return type;
}

QString translatedStatus(const QString &status)
{
    if (status == QStringLiteral("Ready")) {
        return QCoreApplication::translate("Equipment", "Ready");
    }
    if (status == QStringLiteral("Closed")) {
        return QCoreApplication::translate("Equipment", "Closed");
    }
    if (status == QStringLiteral("Open")) {
        return QCoreApplication::translate("Equipment", "Open");
    }
    if (status == QStringLiteral("Energized")) {
        return QCoreApplication::translate("Equipment", "Energized");
    }
    if (status == QStringLiteral("De-energized")) {
        return QCoreApplication::translate("Equipment", "De-energized");
    }
    if (status == QStringLiteral("Loading")) {
        return QCoreApplication::translate("Equipment", "Loading");
    }
    if (status == QStringLiteral("Cooling")) {
        return QCoreApplication::translate("Equipment", "Cooling");
    }
    return status;
}

} // namespace

#include <QMap>

EquipmentTreeModel::EquipmentTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_rootItem(new Equipment("Root"))
{
}

void EquipmentTreeModel::setLayout(const SubstationLayout::Layout &layout)
{
    beginResetModel();
    delete m_rootItem;
    m_rootItem = new Equipment("Root");
    buildTree(layout);
    endResetModel();
}

void EquipmentTreeModel::updateLayout(const SubstationLayout::Layout &layout)
{
    if (m_rootItem->childCount() != layout.nodes.size()) {
        setLayout(layout);
        return;
    }

    for (const SubstationLayout::NodeSpec &node : layout.nodes) {
        Equipment *equipment = equipmentByName(node.id);
        if (!equipment) {
            setLayout(layout);
            return;
        }

        equipment->update(node.type, node.status, node.location, node.description, node.parameters);
        const QModelIndex firstColumn = indexForEquipment(equipment);
        if (firstColumn.isValid()) {
            emit dataChanged(firstColumn, index(firstColumn.row(), columnCount() - 1, firstColumn.parent()),
                             {Qt::DisplayRole});
        }
    }
}

void EquipmentTreeModel::retranslate()
{
    emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
    emit layoutChanged();
}

EquipmentTreeModel::~EquipmentTreeModel()
{
    delete m_rootItem;
}

Equipment *EquipmentTreeModel::equipmentForIndex(const QModelIndex &index) const
{
    return itemFromIndex(index);
}

QModelIndex EquipmentTreeModel::indexForEquipment(Equipment *equipment) const
{
    if (!equipment || equipment == m_rootItem) {
        return QModelIndex();
    }

    return indexForEquipmentRecursive(m_rootItem, equipment, QModelIndex());
}

Equipment *EquipmentTreeModel::equipmentByName(const QString &name) const
{
    return equipmentByNameRecursive(m_rootItem, name);
}

QModelIndex EquipmentTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    Equipment *parentItem = itemFromIndex(parent);
    Equipment *childItem = parentItem->child(row);
    if (!childItem) {
        return QModelIndex();
    }

    return createIndex(row, column, childItem);
}

QModelIndex EquipmentTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    auto *childItem = static_cast<Equipment *>(index.internalPointer());
    Equipment *parentItem = childItem ? childItem->parentItem() : nullptr;

    if (!parentItem || parentItem == m_rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int EquipmentTreeModel::rowCount(const QModelIndex &parent) const
{
    Equipment *parentItem = itemFromIndex(parent);
    return parentItem ? parentItem->childCount() : 0;
}

int EquipmentTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QVariant EquipmentTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole) {
        return QVariant();
    }

    auto *item = static_cast<Equipment *>(index.internalPointer());
    if (!item) {
        return QVariant();
    }

    if (index.column() == 1) {
        return translatedType(item->type());
    }
    if (index.column() == 2) {
        return translatedStatus(item->status());
    }
    return item->data(index.column());
}

QVariant EquipmentTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
    case 0:
        return tr("Equipment");
    case 1:
        return tr("Type");
    case 2:
        return tr("Status");
    default:
        return QVariant();
    }
}

Qt::ItemFlags EquipmentTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

Equipment *EquipmentTreeModel::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid()) {
        return static_cast<Equipment *>(index.internalPointer());
    }
    return m_rootItem;
}

void EquipmentTreeModel::buildTree(const SubstationLayout::Layout &layout)
{
    Equipment *equipment;
    for (const SubstationLayout::NodeSpec &node : layout.nodes) {
        equipment = new Equipment(node.id,
                                node.type,
                                node.status,
                                node.location,
                                node.description,
                                node.parameters,
                                m_rootItem);
        m_rootItem->appendChild(equipment);
    }
}

QModelIndex EquipmentTreeModel::indexForEquipmentRecursive(Equipment *current, Equipment *target, const QModelIndex &currentIndex) const
{
    if (!current || !target) {
        return QModelIndex();
    }
    Equipment *child;
    QModelIndex childIndex;
    QModelIndex nested;
    for (int row = 0; row < current->childCount(); ++row) {
        child = current->child(row);
        childIndex = index(row, 0, currentIndex);
        if (child == target) {
            return childIndex;
        }

        nested = indexForEquipmentRecursive(child, target, childIndex);
        if (nested.isValid()) {
            return nested;
        }
    }

    return QModelIndex();
}

Equipment *EquipmentTreeModel::equipmentByNameRecursive(Equipment *current, const QString &name) const
{
    if (!current) {
        return nullptr;
    }

    if (current->name() == name) {
        return current;
    }
    Equipment *child;
    for (int row = 0; row < current->childCount(); ++row) {
        child = equipmentByNameRecursive(current->child(row), name);
        if (child) {
            return child;
        }
    }

    return nullptr;
}
