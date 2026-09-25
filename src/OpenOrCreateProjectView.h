#pragma once

#include <QWidget>

class OpenOrCreateProjectView final : public QWidget
{
    Q_OBJECT

public:
    explicit OpenOrCreateProjectView(QWidget *parent = nullptr);
    void createNewLayout();

signals:
    void projectTitleAccepted(const QString &title);
    void openLayoutRequested();
    void openLastLayoutRequested();

private:
    void InitUI( );

};
