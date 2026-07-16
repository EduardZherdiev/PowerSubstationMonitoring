#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QMap>
#include <QString>
#include <QVector>

class Equipment {
  public:
    Equipment(const QString& name, const QString& type = QString(), const QString& status = QString(),
              const QString& location = QString(), const QString& description = QString(),
              const QMap<QString, QString>& parameters = QMap<QString, QString>(), Equipment* parent = nullptr);
    ~Equipment();

    void appendChild(Equipment* child);

    Equipment* child(int row) const;
    int childCount() const;
    int row() const;
    Equipment* parentItem() const;

    QString name() const;
    QString type() const;
    QString status() const;
    QString location() const;
    QString description() const;
    const QMap<QString, QString>& parameters() const;
    QString data(int column) const;
    void update(const QString& type, const QString& status, const QString& location, const QString& description,
                const QMap<QString, QString>& parameters);

  private:
    QString m_name;
    QString m_type;
    QString m_status;
    QString m_location;
    QString m_description;
    QMap<QString, QString> m_parameters;
    QVector<Equipment*> m_children;
    Equipment* m_parent;
};

#endif // EQUIPMENT_H
