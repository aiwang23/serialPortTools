//
// Created by wang on 25-4-3.
//

#ifndef OUTTEXTEDIT_H
#define OUTTEXTEDIT_H
#include <ElaPlainTextEdit.h>


class outTextEdit : public ElaPlainTextEdit {
    Q_OBJECT

public:
    explicit outTextEdit(QWidget *parent = nullptr);

    explicit outTextEdit(const QString &text, QWidget *parent = nullptr);

protected:
    virtual void contextMenuEvent(QContextMenuEvent *event) override;
};


#endif //OUTTEXTEDIT_H
