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
#include "qgscamerapose.h"

#include <memory>
#include <QTime>


class Qgs3DMeasureDialog;
class QgsRubberBand3D;
class QgsCameraPose;
class QTimer;

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
    void reset();

    void setupNavigation();

    //! Update values from settings
    void updateSettings();

    void navigateRightSide( float steps );
    void navigateForward( float steps );

    /**
     * Update camero position at the \a newCamPosInMap. 
     * 
     * Elevation will be computed according to terrain elevation at X/Y, vertical scale and terrain offset.
     */
    void updateNavigationCamera( const QgsPoint &newCamPosInMap );

    /**
     * If the event is relevant, handles the event and returns TRUE, otherwise FALSE.
     * \since QGIS 3.44
     */
    bool keyboardEventFilter( QKeyEvent *event );

  signals:
    void finished();

  private slots:
    void handleMarkerMove( const QPoint &screenPos );
    void mouseReleaseEvent( QMouseEvent *event ) override;
    void mouseMoveEvent( QMouseEvent *event ) override;
    void keyPressEvent( QKeyEvent *event ) override;
    void mouseWheelEvent( QWheelEvent *event ) override;
    void refreshCameraForJump();

  private:
    std::unique_ptr<QgsRubberBand3D> mRubberBand;

    QString mPlatformName;
    bool mIsNavigating;
    bool mIsNavigationPaused;
    bool mIsEnabled;
    bool mIgnoreNextMouseMove;
    QgsCameraPose mPreviousCameraPose;
    QgsPoint mMarkerPos;
    QPoint mLasMousePos;
    QTime mLastMarkerTime;
    QgsPoint mLastCamPosInMap;

    QTimer *mJumpTimer;
    QTime mJumpTime;
    double mJumpDefaultHeight = 5.0;
    double mJumpDefaultTime = 100; // msec
    double mJumpAgainTime = 0;
    double mJumpAgainHeight = 0;

    double jumpHeight( double t );
};

#endif // QGS3DMAPTOOLSTREETVIEW_H
