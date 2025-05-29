#include "printservice.h"
#include <QPrinter>
#include <QPainter>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
#include <QPrintDialog>
#include <QObject>

void PrintService::printEquipmentList(const QVector<Equipment>& equipment, QWidget* parent)
{
    QPrinter printer;
    QPrintDialog dialog(&printer, parent);
    if (dialog.exec() != QDialog::Accepted) return;

    QTextDocument doc;
    QTextCursor cursor(&doc);

    // Заголовок
    cursor.insertText(QObject::tr("Construction Equipment Report\n\n"));

    // Таблица
    QTextTableFormat tableFormat;
    tableFormat.setHeaderRowCount(1);
    tableFormat.setAlignment(Qt::AlignHCenter);

    QTextTable *table = cursor.insertTable(equipment.size() + 1, 8, tableFormat);

    // Заголовки столбцов
    table->cellAt(0, 0).firstCursorPosition().insertText(QObject::tr("ID"));
    table->cellAt(0, 1).firstCursorPosition().insertText(QObject::tr("Model"));
    table->cellAt(0, 2).firstCursorPosition().insertText(QObject::tr("Type"));
    table->cellAt(0, 3).firstCursorPosition().insertText(QObject::tr("Year"));
    table->cellAt(0, 4).firstCursorPosition().insertText(QObject::tr("Reg. Number"));
    table->cellAt(0, 5).firstCursorPosition().insertText(QObject::tr("Operating Hours"));
    table->cellAt(0, 6).firstCursorPosition().insertText(QObject::tr("Condition"));
    table->cellAt(0, 7).firstCursorPosition().insertText(QObject::tr("Last Maintenance"));

    // Данные
    for (int i = 0; i < equipment.size(); ++i) {
        const Equipment& eq = equipment[i];
        table->cellAt(i+1, 0).firstCursorPosition().insertText(QString::number(eq.id()));
        table->cellAt(i+1, 1).firstCursorPosition().insertText(eq.model());
        table->cellAt(i+1, 2).firstCursorPosition().insertText(eq.type());
        table->cellAt(i+1, 3).firstCursorPosition().insertText(QString::number(eq.year()));
        table->cellAt(i+1, 4).firstCursorPosition().insertText(eq.regNumber());
        table->cellAt(i+1, 5).firstCursorPosition().insertText(QString::number(eq.operatingHours()));
        table->cellAt(i+1, 6).firstCursorPosition().insertText(eq.condition());
        table->cellAt(i+1, 7).firstCursorPosition().insertText(eq.lastMaintenance().toString("dd.MM.yyyy"));
    }

    doc.print(&printer);
}
