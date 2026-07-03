#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QResizeEvent;
class Equipment;
class EquipmentTreeModel;
class SubstationDiagramView;
class QModelIndex;
enum class EventLevel;
namespace SubstationLayout {
struct Layout;
}

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupModelAndViews();
    void configureParameterPanel();
    void connectInteractions();
    void loadSubstationLayout();
    void displayEquipment(Equipment *equipment, bool fromUserAction);
    void appendEvent(EventLevel level, const QString &source, const QString &message);
    Equipment *equipmentFromTreeIndex(const QModelIndex &index) const;
    Equipment *findEquipmentByName(const QString &name) const;

    Ui::MainWindow *ui;
    EquipmentTreeModel *equipmentModel;
    SubstationDiagramView *diagramView;
};
#endif // MAINWINDOW_H
