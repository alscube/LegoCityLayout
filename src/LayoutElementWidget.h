#pragma once

#include <QPoint>
#include <QString>
#include <QWidget>

class QMouseEvent;

class LayoutElementWidget final : public QWidget
{
public:
    LayoutElementWidget(const QString &name, const QString &resourcePath,
                        QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString m_resourcePath;
    QPoint m_dragStartPosition;
};
