#pragma once

#include <QWidget>

class LayoutElementsView final : public QWidget
{
    Q_OBJECT

public:
    explicit LayoutElementsView(QWidget *parent = nullptr);

    void addLayoutElements();
};
