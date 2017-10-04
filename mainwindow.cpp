#include <QtGui/QTextCharFormat>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QFile>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString db_Name = QCoreApplication::applicationDirPath() + "/Times.sqlite";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_Name);
    if (QFile::exists(db_Name)) {
        db.open();
    } else {
        ui->statusBar->showMessage("Could not open database file Times.sqlite");
        ui->saveButton->setEnabled(false);
        ui->deleteButton->setEnabled(false);
    }

    // Update Calendar
    QSqlQuery query;
    QBrush brush;
    QDate today = QDate::currentDate();
    query.prepare("SELECT * FROM movements WHERE date>=date('now','start of month') AND date<=date('now','start of month','+1 month','-1 day')");
    if (query.exec())
    {
        while (query.next())
        {
            QTime delay = QTime::fromString(query.value("delay").toString(), "hh:mm");
            QTime overtime = QTime::fromString(query.value("overtime").toString(), "hh:mm");
            QDate date = QDate::fromString(query.value("date").toString(), "yyyy-MM-dd");

            // Update delayEdit and overtimeEdit of current day selection
            if (date == today) {
                ui->delayEdit->setTime(delay);
                ui->overtimeEdit->setTime(overtime);
            }

            // Update Calendar Widget
            if (delay < overtime) {
                brush.setColor( Qt::green );
            } else if (delay == overtime){
                brush.setColor( Qt::white );
            } else {
                brush.setColor( Qt::red );
            }

            QTextCharFormat cf = ui->calendar->dateTextFormat( date );
            cf.setBackground( brush );
            ui->calendar->setDateTextFormat( date, cf );

        }
    }

    // Update total section
    updateTotal(QDate::currentDate().toString("yyyy-MM-dd"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateTotal(QString date) {
    QSqlQuery query;
    // Update Total Delay, Total Overtime and Total Difference
    query.prepare("SELECT time(sum(strftime('%s',delay) - strftime('%s', '00:00:00')), 'unixepoch'), time(sum(strftime('%s',overtime) - strftime('%s', '00:00:00')), 'unixepoch') FROM movements WHERE date>=date(:date,'start of month') AND date<=date(:date,'start of month','+1 month','-1 day')");
    query.bindValue(":date", date);
    if (query.exec())
    {
        if (query.next()) {
            QTime totDelay = QTime::fromString(query.value(0).toString());
            QTime totOvertime = QTime::fromString(query.value(1).toString());
            QTime totDiff = QTime(0,0).addSecs(abs(totDelay.secsTo(totOvertime)));

            if (totDelay.isNull()) {
                ui->totDelay->setText("00:00");
            } else {
                ui->totDelay->setText(totDelay.toString("hh:mm"));
            }
            if (totOvertime.isNull()) {
                ui->totOvertime->setText("00:00");
            } else {
                ui->totOvertime->setText(totOvertime.toString("hh:mm"));
            }

            if (totDelay > totOvertime) {
                ui->totDiff->setStyleSheet("color: red;");
            } else {
                ui->totDiff->setStyleSheet("color: green;");
            }
            if (totDiff.isNull()) {
                ui->totDiff->setText("00:00");
            } else {
                ui->totDiff->setText(totDiff.toString("hh:mm"));
            }
        }
    }
}

void MainWindow::on_saveButton_clicked()
{
    // Variables
    QTime delay = ui->delayEdit->time();
    QTime overtime = ui->overtimeEdit->time();
    QDate date = ui->calendar->selectedDate();
    QBrush brush;
    QSqlQuery query;

    // Update Calendar Widget
    if (delay < overtime) {
        brush.setColor( Qt::green );
    } else if (delay == overtime){
        brush.setColor( Qt::white );
    } else {
        brush.setColor( Qt::red );
    }

    QTextCharFormat cf = ui->calendar->dateTextFormat( date );
    cf.setBackground( brush );
    ui->calendar->setDateTextFormat( date, cf );


    // Check if record already exists in database
    query.prepare("SELECT date FROM movements WHERE date=:date");
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    if (query.exec())
    {
        if (query.next())
        {
            // If record exists we need to update it
            query.prepare("UPDATE movements SET delay=:delay, overtime=:overtime WHERE date=:date");
        } else {
            query.prepare("INSERT INTO movements (date, delay, overtime) VALUES (:date, :delay, :overtime)");
        }
    }
    // Store in database
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    query.bindValue(":delay", delay.toString("hh:mm"));
    query.bindValue(":overtime", overtime.toString("hh:mm"));
    if(query.exec())
    {
        ui->statusBar->showMessage("Movements saved successful in database");
    }
    else
    {
        ui->statusBar->showMessage(query.lastError().text());
    }

    // Update Total section
    updateTotal(date.toString("yyyy-MM-dd"));
}

void MainWindow::on_calendar_selectionChanged()
{
    // Get records from database
    QDate date = ui->calendar->selectedDate();
    QSqlQuery query;
    query.prepare("SELECT delay, overtime FROM movements WHERE date=:date");
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    if (query.exec())
    {
        if (query.next())
        {
            ui->delayEdit->setTime(QTime::fromString(query.value("delay").toString(), "hh:mm"));
            ui->overtimeEdit->setTime(QTime::fromString(query.value("overtime").toString(), "hh:mm"));
        } else {
            ui->delayEdit->setTime(QTime(0,0));
            ui->overtimeEdit->setTime(QTime(0,0));
        }
    }
}

void MainWindow::on_calendar_currentPageChanged(int year, int month)
{
    QDate start(year, month, 1);
    QDate end(year, month, start.daysInMonth());
    QSqlQuery query;
    QBrush brush;
    query.prepare("SELECT * FROM movements WHERE date>=date(:start,'-15 days') AND date<=date(:end,'+15 days')");
    query.bindValue(":start", start.toString("yyyy-MM-dd"));
    query.bindValue(":end", end.toString("yyyy-MM-dd"));
    if (query.exec())
    {
        while (query.next())
        {
            QTime delay = QTime::fromString(query.value("delay").toString(), "hh:mm");
            QTime overtime = QTime::fromString(query.value("overtime").toString(), "hh:mm");
            QDate date = QDate::fromString(query.value("date").toString(), "yyyy-MM-dd");

            if (date >= start && date <= end) {
                // Update Calendar Widget
                if (delay < overtime) {
                    brush.setColor( Qt::green );
                } else if (delay == overtime){
                    brush.setColor( Qt::white );
                } else {
                    brush.setColor( Qt::red );
                }
            } else {
                brush.setColor( Qt::white );
            }


            QTextCharFormat cf = ui->calendar->dateTextFormat( date );
            cf.setBackground( brush );
            ui->calendar->setDateTextFormat( date, cf );
        }
    }

    ui->calendar->setSelectedDate(start);

    // Update Total section
    updateTotal(start.toString("yyyy-MM-dd"));
}

void MainWindow::on_deleteButton_clicked()
{
    // Variables
    QDate date = ui->calendar->selectedDate();
    QBrush brush;
    QSqlQuery query;

    // Update Calendar Widget
    brush.setColor( Qt::white );
    QTextCharFormat cf = ui->calendar->dateTextFormat( date );
    cf.setBackground( brush );
    ui->calendar->setDateTextFormat( date, cf );

    // Remove record from database
    query.prepare("DELETE FROM movements WHERE date=:date");
    query.bindValue(":date", date.toString("yyyy-MM-dd"));
    if (query.exec())
    {
        ui->statusBar->showMessage("Movements deleted successfully in database");
    }

    // Update Total section
    updateTotal(date.toString("yyyy-MM-dd"));

    // Clear textEdit
    ui->delayEdit->setTime(QTime(0,0));
    ui->overtimeEdit->setTime(QTime(0,0));
}
