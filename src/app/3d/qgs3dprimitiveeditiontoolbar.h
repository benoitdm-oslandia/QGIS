/***************************************************************************
    qgs3dprimitiveeditiontoolbar.h
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

#ifndef QGS3DPRIMITIVEEDITIONTOOLBAR_H
#define QGS3DPRIMITIVEEDITIONTOOLBAR_H

#include "qgs3deditiontoolbar.h"
#include "qgs3dsnappingmanager.h"

class Qgs3DMapToolCreatePrimitive;

class Qgs3DPrimitiveEditionToolBar : public Qgs3DEditionToolBar
{
    Q_OBJECT

  public:
    Qgs3DPrimitiveEditionToolBar( Qgs3DMapCanvasWidget *parent );
    bool accept( QgsMapLayer *layer ) const override;
    void activate( QgsMapLayer *layer ) override;
    void deactivate() override;
    QList<QAction *> groupActions() const override;

  private slots:
    void createCube();
    void createSphere();

  private:
    QAction *mCreatePrimitiveAction = nullptr;
    QAction *mSnappingAction = nullptr;
    QList<QAction *> mActions;

    Qgs3DMapToolCreatePrimitive *mCreatePrimitiveMapTool = nullptr;
    std::unique_ptr<Qgs3DSnappingManager> mSnapper = nullptr;
};

#endif // QGS3DPRIMITIVEEDITIONTOOLBAR_H
