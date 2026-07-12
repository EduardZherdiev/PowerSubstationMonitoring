#include "equipment.h"

#include <QtAlgorithms>

Equipment::Equipment(const QString &name,
                     const QString &type,
                     const QString &status,
                     const QString &location,
                     const QString &description,
                     const QMap<QString, QString> &parameters,
                     Equipment *parent)
    : m_name(name)
    , m_type(type)
    , m_status(status)
    , m_location(location)
    , m_description(description)
    , m_parameters(parameters)
    , m_parent(parent)
{
}

Equipment::~Equipment()
{
    qDeleteAll(m_children);
}

void Equipment::appendChild(Equipment *child)
{
    if (child) {
        child->m_parent = this;
        m_children.append(child);
    }
}

Equipment *Equipment::child(int row) const
{
    if (row < 0 || row >= m_children.size()) {
        return nullptr;
    }
    return m_children.at(row);
}

int Equipment::childCount() const
{
    return m_children.size();
}

int Equipment::row() const
{
    if (!m_parent) {
        return 0;
    }
    return m_parent->m_children.indexOf(const_cast<Equipment *>(this));
}

Equipment *Equipment::parentItem() const
{
    return m_parent;
}

QString Equipment::name() const
{
    return m_name;
}

QString Equipment::type() const
{
    return m_type;
}

QString Equipment::status() const
{
    return m_status;
}

QString Equipment::location() const
{
    return m_location;
}

QString Equipment::description() const
{
    return m_description;
}

const QMap<QString, QString> &Equipment::parameters() const
{
    return m_parameters;
}

QString Equipment::data(int column) const
{
    switch (column) {
    case 0:
        return m_name;
    case 1:
        return m_type;
    case 2:
        return m_status;
    default:
        return QString();
    }
}

void Equipment::update(const QString &type,
                       const QString &status,
                       const QString &location,
                       const QString &description,
                       const QMap<QString, QString> &parameters)
{
    m_type = type;
    m_status = status;
    m_location = location;
    m_description = description;
    m_parameters = parameters;
}
