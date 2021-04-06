/* Copyright (C) 2004-2021 Robert Griebl. All rights reserved.
**
** This file is part of BrickStore.
**
** This file may be distributed and/or modified under the terms of the GNU
** General Public License version 2 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://fsf.org/licensing/licenses/gpl.html for GPL licensing information.
*/
#pragma once

#include <QWizard>
#include <QVector>
#include <QHash>

#include "common/documentmodel.h"
#include "ui_consolidatedialog.h"

class View;


class ConsolidateDialog : public QWizard, private Ui::ConsolidateDialog
{
    Q_OBJECT
public:
    explicit ConsolidateDialog(View *view, QVector<DocumentModel::Consolidate> &list, bool addItems);
    ~ConsolidateDialog() override;

protected:
    int nextId() const override;
    void initializeCurrentPage();
    bool validateCurrentPage() override;
    void showIndividualMerge(int idx);
    int destinationIndex(const LotList &lots, DocumentModel::MergeMode);

signals:

private:
    enum class Destination {
        Not = -1,
        IntoTopSorted = 0,
        IntoBottomSorted = 1,
        IntoLowestIndex = 2,
        IntoHighestIndex = 3,
        IntoExisting = 4,
        IntoNew = 5
    };

    int calculateIndex(const DocumentModel::Consolidate &consolidate, Destination destination);

    bool m_addingItems = false;
    bool m_forAll = true;
    int m_individualIdx = -1;

    BrickLink::LotList m_documentLots;
    BrickLink::LotList m_documentSortedLots;

    QVector<DocumentModel::Consolidate> &m_list;

    // default settings for individual merge
    DocumentModel::FieldMergeModes m_fieldMergeModes;
    bool m_doNotDeleteEmpty = false;
    Destination m_destination = Destination::Not;

    static QString s_baseConfigPath;
};
