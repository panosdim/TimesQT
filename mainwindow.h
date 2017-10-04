#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_saveButton_clicked();

    void on_calendar_selectionChanged();

    void on_calendar_currentPageChanged(int year, int month);

    void updateTotal(QString date);

    void on_deleteButton_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
