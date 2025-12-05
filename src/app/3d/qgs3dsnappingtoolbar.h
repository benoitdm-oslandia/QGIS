/***************************************************************************
    qgs3dsnappingtoolbar.h
    -------------------
    begin                : December 2025
    copyright            : (C) 2025 Oslandia
    email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DSNAPPINGTOOLBAR_H
#define QGS3DSNAPPINGTOOLBAR_H

#include "qgs3dmapcanvaswidget.h"

#include <QToolBar>
#include <QObject>
#include <QLabel>

class QgsSettings;
class Qgs3DSnappingManager;

class Qgs3DSnappingToolbar : public QToolBar
{
  public:
    Qgs3DSnappingToolbar( Qgs3DMapCanvasWidget *parent, Qgs3DSnappingManager *snapper, const QgsSettings &setting );

  public slots:
    void onSnappingButtonTriggered( QAction *action );
    void onSettingUpdate( const Qgs3DMapSettings *map );

  private:
    Qgs3DSnappingManager *mSnapper = nullptr;
    QAction *mSnappingAction = nullptr;
    QLabel *mSnappingUnitLabel = nullptr;
    QgsDoubleSpinBox *mSnappingToleranceSpinBox;
};

#endif // QGS3DSNAPPINGTOOLBAR_H
