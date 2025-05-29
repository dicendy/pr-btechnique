#ifndef PRINTSERVICE_H
#define PRINTSERVICE_H

#include <QVector>
#include "equipment.h"

class QWidget;

class PrintService
{
public:
    static void printEquipmentList(const QVector<Equipment>& equipment, QWidget* parent);
};

#endif // PRINTSERVICE_H
