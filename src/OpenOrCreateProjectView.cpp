
// Copyright 2026. Alan Krzywicki

// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
#include "OpenOrCreateProjectView.h"

#include <QColor>
#include <QInputDialog>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>

OpenOrCreateProjectView::OpenOrCreateProjectView(QWidget *parent)
    : QWidget(parent)
{
    InitUI();
}


void OpenOrCreateProjectView::InitUI( )
{
    QPalette startupPalette = palette();
    startupPalette.setColor(QPalette::Window, QColor(52, 52, 52));
    setAutoFillBackground(true);
    setPalette(startupPalette);

    auto *viewLayout = new QVBoxLayout(this);
    viewLayout->addStretch();

    auto *buttonPanel = new QWidget(this);
    buttonPanel->setMaximumWidth(280);
    auto *buttonLayout = new QVBoxLayout(buttonPanel);
    buttonLayout->setSpacing(12);

    auto *createButton = new QPushButton(tr("Create New Layout"), buttonPanel);
    auto *openButton = new QPushButton(tr("Open Layout"), buttonPanel);
    auto *openLastButton = new QPushButton(tr("Open Last Layout"), buttonPanel);
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(openLastButton);

    connect(createButton, &QPushButton::clicked,
            this, &OpenOrCreateProjectView::createNewLayout);
    connect(openButton, &QPushButton::clicked,
            this, &OpenOrCreateProjectView::openLayoutRequested);
    connect(openLastButton, &QPushButton::clicked,
            this, &OpenOrCreateProjectView::openLastLayoutRequested);

    viewLayout->addWidget(buttonPanel, 0, Qt::AlignHCenter);
    viewLayout->addStretch();
}


void OpenOrCreateProjectView::createNewLayout()
{
    QInputDialog dialog(this);
    dialog.setWindowTitle(tr("New Layout"));
    dialog.setLabelText(tr("Layout title:"));
    dialog.setInputMode(QInputDialog::TextInput);
    dialog.setTextEchoMode(QLineEdit::Normal);
    dialog.adjustSize();

    QWidget *projectView = parentWidget();
    const QPoint projectCenter = projectView
        ? projectView->mapToGlobal(projectView->rect().center())
        : mapToGlobal(rect().center());
    dialog.move(projectCenter - dialog.rect().center());

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString title = dialog.textValue().trimmed();

    if (title.isEmpty()) {
        return;
    }

    emit projectTitleAccepted(title);
}
