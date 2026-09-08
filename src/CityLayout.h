#pragma once

#include <QWidget>

class QDragEnterEvent;
class QDropEvent;

class CityLayout final : public QWidget
{
    Q_OBJECT

public:
    explicit CityLayout(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};
