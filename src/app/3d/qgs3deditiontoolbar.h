/***************************************************************************
    qgs3deditiontoolbar.h
    -------------------
    begin                : November 2025
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

#ifndef QGS3DEDITIONTOOLBAR_H
#define QGS3DEDITIONTOOLBAR_H

#include "qgs3dmapcanvaswidget.h"

class QAction;

class Qgs3DEditionToolBar : public QToolBar
{
    Q_OBJECT

  public:
    Qgs3DEditionToolBar( const QString &title, Qgs3DMapCanvasWidget *parent );

    virtual bool accept( QgsMapLayer *layer ) const = 0;
    virtual void activate( QgsMapLayer *layer ) = 0;
    virtual void deactivate() = 0;
    virtual QList<QAction *> groupActions() const = 0;

  protected:
    Qgs3DMapCanvasWidget *mParentWidget;
};

#endif // QGS3DEDITIONTOOLBAR_H
