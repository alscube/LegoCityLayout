#pragma once

#include <QPoint>
#include <QString>
#include <QSize>
#include <QWidget>

class QMouseEvent;

class LayoutElement final : public QWidget
{
public:
    LayoutElement(const QString &name, const QString &resourcePath,
                  const QSize &plateSize, QWidget *parent = nullptr);

    QSize plateSize() const { return m_plateSize; }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString m_name;
    QString m_resourcePath;
    QSize m_plateSize; // Width and height in studs.
    QPoint m_dragStartPosition;
};
