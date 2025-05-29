#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("About"));
    ui->authorLabel->setTextFormat(Qt::RichText);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this); // Вызываем retranslateUi для обновления всех элементов
        setWindowTitle(tr("About")); // Обновляем заголовок, так как он устанавливается вручную
    }
    QDialog::changeEvent(event); // Вызываем базовую реализацию
}
