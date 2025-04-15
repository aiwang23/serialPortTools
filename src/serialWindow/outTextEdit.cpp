//
// Created by wang on 25-4-3.
//

#include "outTextEdit.h"

#include <ELaMenu.h>
#include <QClipboard>
#include <QGuiApplication>


outTextEdit::outTextEdit(QWidget *parent): ElaPlainTextEdit(parent) {
}

outTextEdit::outTextEdit(const QString &text, QWidget *parent): ElaPlainTextEdit(text, parent) {
}

void outTextEdit::contextMenuEvent(QContextMenuEvent *event) {
    ElaMenu *menu = new ElaMenu(this);
    menu->setMenuItemHeight(27);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QAction *action = nullptr;

    action = menu->addElaIconAction(ElaIconType::Copy, tr("copy"), QKeySequence::Copy);
    action->setEnabled(!textCursor().selectedText().isEmpty());
    connect(action, &QAction::triggered, this, &ElaPlainTextEdit::copy);

    menu->addSeparator();
    action = menu->addAction(tr("select all"));
    action->setShortcut(QKeySequence::SelectAll);
    action->setEnabled(!toPlainText().isEmpty() && !(textCursor().selectedText() == toPlainText()));
    connect(action, &QAction::triggered, this, &ElaPlainTextEdit::selectAll);

    menu->addSeparator();
    action = menu->addAction(tr("clear"));
    action->setShortcut(QKeySequence::Delete);
    connect(action, &QAction::triggered, this, &QPlainTextEdit::clear);

    menu->popup(event->globalPos());
    this->setFocus();
}
