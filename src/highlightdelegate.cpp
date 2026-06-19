#include "highlightdelegate.h"

#include <QApplication>
#include <QPainter>
#include <QStyleOptionViewItem>

HighlightDelegate::HighlightDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {
}

void HighlightDelegate::setSearchText(const QString &text) {
    searchText_ = text;
}

void HighlightDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const {
    painter->save();

    QStyleOptionViewItem opt = option;
    opt.state &= ~QStyle::State_HasFocus;

    QString text = index.data(Qt::DisplayRole).toString();
    QRect textRect = QApplication::style()->itemTextRect(opt.fontMetrics, opt.rect, Qt::AlignLeft,
                                                           opt.state & QStyle::State_Enabled, text);

    QColor textColor;
    if (opt.state & QStyle::State_Selected) {
        textColor = opt.palette.color(QPalette::HighlightedText);
    } else {
        textColor = opt.palette.color(QPalette::Text);
    }

    painter->setPen(textColor);

    if (searchText_.isEmpty()) {
        painter->drawText(textRect, text);
    } else {
        int pos = 0;
        int searchTextLen = searchText_.length();
        QString lowerText = text.toLower();
        QString lowerSearch = searchText_.toLower();

        while (true) {
            int matchPos = lowerText.indexOf(lowerSearch, pos);
            if (matchPos < 0) {
                break;
            }

            if (matchPos > pos) {
                QRect nonMatchRect = textRect;
                nonMatchRect.setX(nonMatchRect.x() + opt.fontMetrics.horizontalAdvance(text.left(pos)));
                nonMatchRect.setWidth(opt.fontMetrics.horizontalAdvance(text.mid(pos, matchPos - pos)));
                painter->drawText(nonMatchRect, text.mid(pos, matchPos - pos));
            }

            QRect matchRect = textRect;
            matchRect.setX(matchRect.x() + opt.fontMetrics.horizontalAdvance(text.left(matchPos)));
            matchRect.setWidth(opt.fontMetrics.horizontalAdvance(text.mid(matchPos, searchTextLen)));

            QColor matchColor = textColor;
            if (!(opt.state & QStyle::State_Selected)) {
                matchColor = Qt::yellow;
            }
            painter->setPen(matchColor);
            painter->drawText(matchRect, text.mid(matchPos, searchTextLen));

            pos = matchPos + searchTextLen;
        }

        if (pos < text.length()) {
            QRect remainingRect = textRect;
            remainingRect.setX(remainingRect.x() + opt.fontMetrics.horizontalAdvance(text.left(pos)));
            remainingRect.setWidth(opt.fontMetrics.horizontalAdvance(text.mid(pos)));
            painter->drawText(remainingRect, text.mid(pos));
        }
    }

    painter->restore();
}
