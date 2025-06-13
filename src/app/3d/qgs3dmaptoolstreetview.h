/***************************************************************************
  qgs3dmaptoolstreetview.h
  --------------------------------------
  Date                 : Jun 2019
  Copyright            : (C) 2019 by Ismail Sunni
  Email                : imajimatika at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DMAPTOOLSTREETVIEW_H
#define QGS3DMAPTOOLSTREETVIEW_H

#include "qgs3dmaptool.h"
#include "qgspoint.h"

#include <memory>


class Qgs3DMeasureDialog;
class QgsRubberBand3D;

class Qgs3DMapToolStreetView : public Qgs3DMapTool
{
    Q_OBJECT

  public:
    Qgs3DMapToolStreetView( Qgs3DMapCanvas *canvas );
    ~Qgs3DMapToolStreetView() override;

    void activate() override;
    void deactivate() override;

    QCursor cursor() const override;

    //! Reset and start new
    void restart();

    //! Setup pin point.
    void setupPinPoint( const QgsPoint &point );

    void setupNavigation();

    //! Update values from settings
    void updateSettings();

    void navigateOnX( float steps );
    void navigateOnY( float steps );
    /**
     * Update camero position at the \a newCamPosInMap. 
     * 
     * Elevation will be computed according to terrain elevation at X/Y, vertical scale and terrain offset.
     */
    void updateNavigationCamera( const QgsPoint &newCamPosInMap );

    void quit();

  signals:
    void finished();

  private slots:
    void handleClick( const QPoint &screenPos );
    // void mousePressEvent( QMouseEvent *event ) override;
    void mouseReleaseEvent( QMouseEvent *event ) override;
    void mouseMoveEvent( QMouseEvent *event ) override;
    void keyPressEvent( QKeyEvent *event ) override;
    void mouseWheelEvent( QWheelEvent *event ) override;

  private:
    std::unique_ptr<QgsRubberBand3D> mRubberBand;

    bool mIsNavigating;
    QgsCameraPose mPreviousCameraPose;
    QgsPoint mSelectedPos;
    QPoint mMouseMovePos;
    QTime mLastTime;
};

#endif // QGS3DMAPTOOLSTREETVIEW_H
