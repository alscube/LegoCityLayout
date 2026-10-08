#pragma once

#include <QPoint>
#include <QString>
#include <QSize>
#include <QWidget>

class QMouseEvent;

// Plates
//       Each stud measures 8 mm or 0.8 cm center to center

//       Equivalent to 1/3 of a standard LEGO plate (~0.32 cm / 0.1 in.)
//
// 8x16 - 128 studs
//      ~ 2.5x5 (or 2.6 in. x 5.1 in.)
//      ~ 6.4x12.8 (or 6.4 cm x 12.8 cm)
//
// 16x32 - 512 studs
//       ~ 5x10 (or 5.1 in. x 10.1 in.)
//       ~ 12x25 (or 12.8 cm x 25.6 cm)
//
// 32x32 - 1,024 studs
//       ~10x10 (or 10.1 in. x 10.1 in.),
//       ~25x25 (or 25.6 cm x 25.6 cm)
//
// 48x48 - 1,764 studs
//       ~13x13 (13.2 inches × 13.2 inches)
//       ~33x33 (33.6 cm × 33.6 cm)

// Track (stright)
// Length: 16 studs (128 mm / ~5 inches)
// Width: 8 studs (64 mm / ~2.5 inches across the plastic ties/sleepers)


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
