// -*- coding: utf-8 -*-
// Copyright (C) 2006-2010 Rosen Diankov (rdiankov@cs.cmu.edu)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include "commonmanipulation.h"
#include <boost/algorithm/string/replace.hpp>

#define CALIBRATION_TIMING_DEBUG // uncomment this to get more information about time spent for each stage

void SerializeTransform(std::stringstream& ss, const Transform& t) {
    ss << t.rot.x << ", " << t.rot.y << ", " << t.rot.z << ", " << t.rot.w << ", " << t.trans.x << ", " << t.trans.y << ", " << t.trans.z;
}

void SerializeVector3(std::stringstream& ss, const Vector& v) {
    ss << v.x << ", " << v.y << ", " << v.z;
}

void SerializeVector4(std::stringstream& ss, const Vector& v) {
    ss << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
}

/// \brief Sample rays from the projected OBB on an image plane and returns true if the test function returns true for
///        all the rays. Otherwise, return false
/// \param obb The OBB of targetlink whose transform is set to Ttargetlinkincamera
/// \param delta The discretization step size when sampling rays
/// \param testfn A function for testing occlusion for rays
/// \param tcamera Tcamerainworld
/// \param allowableocclusion Allowable percentage of occluded rays
/// \param bUseOnlyTopFace If true, will consider only the projection of the top face of the OBB. Otherwise, will consider three faces.
bool SampleProjectedOBBWithTest(const OBB& obb, dReal delta, const boost::function<bool(const Vector&)>& testfn, Transform& tcamera, dReal allowableocclusion=0, bool bUseOnlyTopFace=false)
{
    dReal fscalefactor = 0.95f; // have to make box smaller or else rays might miss
    int numpoints, numfaces;
    std::vector<Vector> vpoints;
    std::vector<int> faceindices;
    if( bUseOnlyTopFace ) {
        // Project only one face of the OBB
        numpoints = 4;
        numfaces  = 1;
        vpoints.resize(numpoints);
        faceindices.resize(numfaces*numpoints);

        vpoints[0] = obb.pos + fscalefactor*(  obb.right*obb.extents.x + obb.up*obb.extents.y - obb.dir*obb.extents.z );
        vpoints[1] = obb.pos + fscalefactor*(  obb.right*obb.extents.x - obb.up*obb.extents.y - obb.dir*obb.extents.z );
        vpoints[2] = obb.pos + fscalefactor*( -obb.right*obb.extents.x + obb.up*obb.extents.y - obb.dir*obb.extents.z );
        vpoints[3] = obb.pos + fscalefactor*( -obb.right*obb.extents.x - obb.up*obb.extents.y - obb.dir*obb.extents.z );

        faceindices[0] = 0; faceindices[1] = 1; faceindices[2] = 2; faceindices[3] = 3;
    }
    else {
        // Project 3 faces of the OBB
        numpoints = 8;
        numfaces  = 3;
        vpoints.resize(numpoints);
        faceindices.resize(numfaces*numpoints);

        vpoints[0] = obb.pos + fscalefactor*(  obb.right*obb.extents.x + obb.up*obb.extents.y + obb.dir*obb.extents.z );
        vpoints[1] = obb.pos + fscalefactor*(  obb.right*obb.extents.x + obb.up*obb.extents.y - obb.dir*obb.extents.z );
        vpoints[2] = obb.pos + fscalefactor*(  obb.right*obb.extents.x - obb.up*obb.extents.y + obb.dir*obb.extents.z );
        vpoints[3] = obb.pos + fscalefactor*(  obb.right*obb.extents.x - obb.up*obb.extents.y - obb.dir*obb.extents.z );
        vpoints[4] = obb.pos + fscalefactor*( -obb.right*obb.extents.x + obb.up*obb.extents.y + obb.dir*obb.extents.z );
        vpoints[5] = obb.pos + fscalefactor*( -obb.right*obb.extents.x + obb.up*obb.extents.y - obb.dir*obb.extents.z );
        vpoints[6] = obb.pos + fscalefactor*( -obb.right*obb.extents.x - obb.up*obb.extents.y + obb.dir*obb.extents.z );
        vpoints[7] = obb.pos + fscalefactor*( -obb.right*obb.extents.x - obb.up*obb.extents.y - obb.dir*obb.extents.z );

        if( obb.right.z >= 0 ) {
            faceindices[0] = 4; faceindices[1] = 5; faceindices[2] = 6; faceindices[3] = 7;
        }
        else {
            faceindices[0] = 0; faceindices[1] = 1; faceindices[2] = 2; faceindices[3] = 3;
        }

        if( obb.up.z >= 0 ) {
            faceindices[4] = 2; faceindices[5] = 3; faceindices[6] = 6; faceindices[7] = 7;
        }
        else {
            faceindices[4] = 0; faceindices[5] = 1; faceindices[6] = 4; faceindices[7] = 5;
        }

        if( obb.dir.z >= 0 ) {
            faceindices[8] = 1; faceindices[9] = 3; faceindices[10] = 5; faceindices[11] = 7;
        }
        else {
            faceindices[8] = 0; faceindices[9] = 2; faceindices[10] = 4; faceindices[11] = 6;
        }
    }

    // Project OBB points onto the plane z = 1 (arbitrary image plane).
    for( int ipoint = 0; ipoint < numpoints; ++ipoint ) {
        dReal fz = 1.0f/vpoints[ipoint].z;
        vpoints[ipoint].x *= fz;
        vpoints[ipoint].y *= fz;
        vpoints[ipoint].z = 1;
    }

    if( 0 ) { // for debugging
        Vector vpoints3d[numpoints];
        for(int j = 0; j < numpoints; ++j) vpoints3d[j] = tcamera*vpoints[j];
        {
            std::stringstream ss; ss << std::setprecision(std::numeric_limits<dReal>::digits10+1);
            ss << "camerapose=array([";
            SerializeTransform(ss, tcamera);
            ss << "]); vpoints=array([";
            std::string delim = "";
            for( int j = 0; j < numpoints; ++j ) {
                ss << delim << "[";
                SerializeVector3(ss, vpoints3d[j]);
                ss << "]";
                delim = ",";
            }
            ss << "]);";
            RAVELOG_DEBUG_FORMAT("%s", ss.str());
        }
    }

    int nallowableoutliers=0;
    if( allowableocclusion > 0 ) {
        // have to compute the area of all the faces!
        dReal farea = 0;
        for(int i = 0; i < numfaces; ++i) {
            Vector v0 = vpoints[faceindices[i*numfaces + 0]];
            Vector v1 = vpoints[faceindices[i*numfaces + 1]] - v0;
            Vector v2 = vpoints[faceindices[i*numfaces + 2]] - v0;
            Vector v = v1.cross(v2);
            farea += v.lengthsqr3();
        }
        nallowableoutliers = (int)(allowableocclusion*farea*0.5/(delta*delta));
    }

    for(int i = 0; i < numfaces; ++i) {
        Vector v0 = vpoints[faceindices[i*numfaces + 0]];
        Vector v1 = vpoints[faceindices[i*numfaces + 1]] - v0;
        Vector v2 = vpoints[faceindices[i*numfaces + 2]] - v0;
        Vector v3 = vpoints[faceindices[i*numfaces + 3]] - v0;
        dReal f3length = RaveSqrt(v3.lengthsqr2());
        Vector v3norm = v3 * (1.0f/f3length);
        Vector v3perp(-v3norm.y,v3norm.x,0,0);
        dReal f1proj = RaveFabs(v3perp.x*v1.x+v3perp.y*v1.y), f2proj = RaveFabs(v3perp.x*v2.x+v3perp.y*v2.y);

        int n1 = (int)(f1proj/delta);
        dReal n1scale = 1.0f/n1;
        Vector vdelta1 = v1*n1scale;
        Vector vdelta2 = (v1-v3)*n1scale;
        dReal fdeltalen = (RaveFabs(v3norm.x*v1.x+v3norm.y*v1.y) + RaveFabs(v3norm.x*(v1.x-v3.x)+v3norm.y*(v1.y-v3.y)))*n1scale;
        dReal ftotalen = f3length;
        Vector vcur1 = v0, vcur2 = v0+v3;
        for(int j = 0; j <= n1; ++j, vcur1 += vdelta1, vcur2 += vdelta2, ftotalen -= fdeltalen ) {
            int numsteps = (int)(ftotalen/delta);
            Vector vdelta = (vcur2-vcur1)*(1.0f/numsteps), vcur = vcur1;
            for(int k = 0; k <= numsteps; ++k, vcur += vdelta) {
                if( !testfn(vcur) ) {
                    if( nallowableoutliers-- <= 0 )
                        return false;
                }
            }
        }

        // Vector vtripoints[6] = {vpoints3d[faceindices[i*numfaces + 0]], vpoints3d[faceindices[i*numfaces + 3]], vpoints3d[faceindices[i*numfaces + 1]],
        //                         vpoints3d[faceindices[i*numfaces + 0]], vpoints3d[faceindices[i*numfaces + 1]], vpoints3d[faceindices[i*numfaces + 3]]};
        // penv->drawtrimesh(vtripoints[0], 16, NULL, 2);

        int n2 = (int)(f2proj/delta);
        if( n2 == 0 )
            continue;

        dReal n2scale = 1.0f/n2;
        vdelta1 = v2*n2scale;
        vdelta2 = (v2-v3)*n2scale;
        fdeltalen = (RaveFabs(v3norm.x*v2.x+v3norm.y*v2.y) + RaveFabs(v3norm.x*(v2.x-v3.x)+v3norm.y*(v2.y-v3.y)))*n2scale;
        ftotalen = f3length;
        vcur1 = v0; vcur2 = v0+v3;
        vcur1 += vdelta1; vcur2 += vdelta2; ftotalen -= fdeltalen; // do one step
        for(int j = 0; j < n2; ++j, vcur1 += vdelta1, vcur2 += vdelta2, ftotalen -= fdeltalen ) {
            int numsteps = (int)(ftotalen/delta);
            Vector vdelta = (vcur2-vcur1)*(1.0f/numsteps), vcur = vcur1;
            for(int k = 0; k <= numsteps; ++k, vcur += vdelta) {
                if( !testfn(vcur) ) {
                    if( nallowableoutliers-- <= 0 )
                        return false;
                }
            }
        }
    }

    return true;
}

class VisualFeedback : public ModuleBase
{
public:
    inline boost::shared_ptr<VisualFeedback> shared_problem() {
        return boost::static_pointer_cast<VisualFeedback>(shared_from_this());
    }
    inline boost::shared_ptr<VisualFeedback const> shared_problem_const() const {
        return boost::static_pointer_cast<VisualFeedback const>(shared_from_this());
    }
    friend class VisibilityConstraintFunction;

    //        void Init(const SensorBase::CameraIntrinsics& KK,int width, int height)
    //    {
    //        ffovx = atanf(0.5f*width/KK.fx);
    //        fcosfovx = cosf(ffovx); fsinfovx = sinf(ffovx);
    //        ffovy = atanf(0.5f*height/KK.fy);
    //        fcosfovy = cosf(ffovy); fsinfovy = sinf(ffovy);
    //    }

    class VisibilityConstraintFunction
    {
        class SampleRaysScope
        {
public:
            SampleRaysScope(VisibilityConstraintFunction& vcf) : _vcf(vcf) {
                _vcf._bSamplingRays = true;
            }
            ~SampleRaysScope() {
                _vcf._bSamplingRays = false;
            }
private:
            VisibilityConstraintFunction& _vcf;
        };
public:
        static void GetAABBFromOBB(const OBB& obb, Vector& vMin, Vector& vMax)
        {
            vMax.x = fabsf(obb.right.x) * obb.extents.x + fabsf(obb.up.x) * obb.extents.y + fabsf(obb.dir.x) * obb.extents.z;
            vMax.y = fabsf(obb.right.y) * obb.extents.x + fabsf(obb.up.y) * obb.extents.y + fabsf(obb.dir.y) * obb.extents.z;
            vMax.z = fabsf(obb.right.z) * obb.extents.x + fabsf(obb.up.z) * obb.extents.y + fabsf(obb.dir.z) * obb.extents.z;
            vMin = obb.pos - vMax;
            vMax += obb.pos;
        }

        VisibilityConstraintFunction(boost::shared_ptr<VisualFeedback> vf) : _vf(vf) {
            _report.reset(new CollisionReport());

            // create the dummy box
            _vTargetLocalOBBs.reserve(1);

            if( !_vf->_targetlink->IsVisible() ) {
                throw OPENRAVE_EXCEPTION_FORMAT("no geometries target link %s is visible so cannot use it for visibility checking", _vf->_targetlink->GetName(), ORE_InvalidArguments);
            }

            // We assume only one geometry named exactly _vf->_targetGeomName exists on
            // the targetlink. This geometry name is hard-coded in
            // handeyecalibrationtask.py in planning common. Here the
            // calibboard geometry OBB is computed relative to the targetlink.
            for(size_t igeom = 0; igeom < _vf->_targetlink->GetGeometries().size(); ++igeom) {
                KinBody::Link::GeometryPtr pgeom = _vf->_targetlink->GetGeometries().at(igeom);
                if( pgeom->IsVisible() && (_vf->_targetGeomName.size() == 0 || pgeom->GetName() == _vf->_targetGeomName) ) {
                    _vTargetLocalOBBs.push_back(geometry::OBBFromAABB(pgeom->ComputeAABB(Transform()), Transform()));
                    break;
                }
            }

            if (_vTargetLocalOBBs.size() == 0 ) {
                std::stringstream ss;
                for(size_t igeom = 0; igeom < _vf->_targetlink->GetGeometries().size(); ++igeom) {
                    ss << _vf->_targetlink->GetGeometries().at(igeom)->GetName() << " (" << _vf->_targetlink->GetGeometries().at(igeom)->IsVisible() << "), ";
                }
                throw OPENRAVE_EXCEPTION_FORMAT("No visibile geometries named '%s' found in target link %s with geometires [%s].", _vf->_targetGeomName%_vf->_targetlink->GetName()%ss.str(), ORE_InvalidArguments);
            }

            vector<AABB> vboxes;
            if( _vTargetLocalOBBs.size() > 0 ) {
                Vector vmin, vmax;
                GetAABBFromOBB(_vTargetLocalOBBs.at(0), vmin, vmax);
                for(size_t iobb = 1; iobb < _vTargetLocalOBBs.size(); ++iobb) {
                    Vector vmin2, vmax2;
                    GetAABBFromOBB(_vTargetLocalOBBs.at(iobb), vmin2, vmax2);
                    if( vmin.x > vmin2.x ) {
                        vmin.x = vmin2.x;
                    }
                    if( vmin.y > vmin2.y ) {
                        vmin.y = vmin2.y;
                    }
                    if( vmin.z > vmin2.z ) {
                        vmin.z = vmin2.z;
                    }
                    if( vmax.x < vmax2.x ) {
                        vmax.x = vmax2.x;
                    }
                    if( vmax.y < vmax2.y ) {
                        vmax.y = vmax2.y;
                    }
                    if( vmax.z < vmax2.z ) {
                        vmax.z = vmax2.z;
                    }
                }

                _abTarget.pos = 0.5*(vmin + vmax);
                _abTarget.extents = vmax - _abTarget.pos;
                _abTarget.extents.x += 0.0001;
                _abTarget.extents.y += 0.0001;
                _abTarget.extents.z += 0.0001;
                vboxes.push_back(_abTarget);
            }

            // have to increase its dimensions a little!

            _ptargetbox = RaveCreateKinBody(_vf->_targetlink->GetParent()->GetEnv());
            _ptargetbox->InitFromBoxes(vboxes,true);
            _ptargetbox->SetName("__visualfeedbacktest__");
            _ptargetbox->GetEnv()->Add(_ptargetbox,true); // need to set to visible, otherwise will be ignored
            _ptargetbox->Enable(false);
            _ptargetbox->SetTransform(_vf->_targetlink->GetTransform());

            _ikreturn.reset(new IkReturn(IKRA_Success));
            _bSamplingRays = false;
            if( _vf->_bIgnoreSensorCollision && !!_vf->_sensorrobot ) {
                _collisionfn = _vf->_targetlink->GetParent()->GetEnv()->RegisterCollisionCallback(boost::bind(&VisibilityConstraintFunction::_IgnoreCollisionCallback,this,_1,_2));
            }
        }

        virtual ~VisibilityConstraintFunction() {
            _ptargetbox->GetEnv()->Remove(_ptargetbox);
        }

        virtual bool IsVisible(bool bcheckocclusion, bool bOutputError, std::string& errormsg)
        {
            Transform ttarget = _vf->_targetlink->GetTransform();
            TransformMatrix tcameraintarget = ttarget.inverse()*_vf->_psensor->GetTransform();
            if( !InConvexHull(tcameraintarget) ) {
                RAVELOG_WARN("box not in camera vision hull (shouldn't happen due to preprocessing\n");
                return false;
            }
            if( bcheckocclusion && IsOccluded(tcameraintarget, bOutputError, errormsg) ) {
                return false;
            }
            return true;
        }

        virtual bool Constraint(const PlannerBase::PlannerParameters::CheckPathConstraintFn& oldfn, const vector<dReal>& pSrcConf, const vector<dReal>& pDestConf, IntervalType interval, PlannerBase::ConfigurationListPtr configurations)
        {
            if( !oldfn(pSrcConf,pDestConf,interval,configurations) ) {
                return false;
            }
            std::string errormsg;
            return IsVisible(true, false, errormsg);
        }

        /// samples the ik
        /// If camera is attached to robot, assume target is not movable and t is the camera position.
        /// If camera is not attached to robot, assume target is movable and t is the target position.
        bool SampleWithCamera(const TransformMatrix& t, vector<dReal>& pNewSample, bool bOutputError, std::string& errormsg)
        {
            Transform tCameraInTarget, ttarget;
            if( _vf->_robot != _vf->_sensorrobot ) {
                ttarget = t;
                tCameraInTarget = ttarget.inverse()*_vf->_psensor->GetTransform();
            }
            else {
                ttarget = _vf->_targetlink->GetTransform();
                tCameraInTarget = ttarget.inverse()*t;
            }
            if( !InConvexHull(tCameraInTarget) ) {
                RAVELOG_DEBUG("box not in camera vision hull\n");
                if( bOutputError ) {
                    errormsg = str(boost::format("{\"type\":\"outofcamera\"}"));
                }

                return false;
            }

            // object is inside, find an ik solution
            Transform tmanipgoal = t*_vf->_tToManip;
            if( !_vf->_pmanip->FindIKSolution(tmanipgoal,IKFO_CheckEnvCollisions, _ikreturn) ) {
                if( bOutputError ) {
                    errormsg = str(boost::format("{\"type\":\"ikfailed\", \"action\":\"0x%x\"}")%_ikreturn->_action);
                }
                RAVELOG_VERBOSE_FORMAT("no valid ik 0x%.8x", _ikreturn->_action);
                return false;
            }
            _vsolution.swap(_ikreturn->_vsolution); // for now swap until this->_vsolution is removed

            // convert the solution into active dofs
            _vf->_robot->GetActiveDOFValues(pNewSample);
            FOREACHC(itarm,_vf->_pmanip->GetArmIndices()) {
                vector<int>::const_iterator itactive = find(_vf->_robot->GetActiveDOFIndices().begin(),_vf->_robot->GetActiveDOFIndices().end(),*itarm);
                if( itactive != _vf->_robot->GetActiveDOFIndices().end() ) {
                    pNewSample.at((int)(itactive-_vf->_robot->GetActiveDOFIndices().begin())) = _vsolution.at((int)(itarm-_vf->_pmanip->GetArmIndices().begin()));
                }
            }
            _vf->_robot->SetActiveDOFValues(pNewSample);

            return !IsOccluded(tCameraInTarget, bOutputError, errormsg);
        }

        /// \brief checks if the target geometries of the target link are inside the camera visiblity convex hull (
        /// \param tCameraInTarget in target coordinate system
        /// \param mindist Minimum distance to keep from the plane (should be non-negative)
        bool InConvexHull(const TransformMatrix& tCameraInTarget, dReal mindist=0)
        {
            _vconvexplanes3d.resize(_vf->_vconvexplanes.size());
            for(size_t i = 0; i < _vf->_vconvexplanes.size(); ++i) {
                _vconvexplanes3d[i] = tCameraInTarget.rotate(_vf->_vconvexplanes[i]);
                _vconvexplanes3d[i].w = -tCameraInTarget.trans.dot3(_vconvexplanes3d[i]) - mindist;
            }
            FOREACH(itobb,_vTargetLocalOBBs) {
                if( !geometry::IsOBBinConvexHull(*itobb,_vconvexplanes3d) ) {
                    return false;
                }
            }
            return true;
        }

        /// check if any part of the environment or robot is in front of the camera blocking the object
        /// sample object's surface and shoot rays
        /// \param tCameraInTarget in target coordinate system
        bool IsOccluded(const TransformMatrix& tCameraInTarget, bool bOutputError, std::string& errormsg)
        {
            KinBody::KinBodyStateSaver saver1(_ptargetbox), saver2(_vf->_targetlink->GetParent(),KinBody::Save_LinkEnable);
            TransformMatrix tCameraInTargetinv = tCameraInTarget.inverse();
            Transform ttarget = _vf->_targetlink->GetTransform();
            _ptargetbox->SetTransform(ttarget); // world
            Transform tworldcamera = ttarget*tCameraInTarget;  // tCameraInTarget is in targetLink coordinates
            _ptargetbox->Enable(true);
            SampleRaysScope srs(*this);
            std::string occludingbodyandlinkname = "";
            FOREACH(itobb,_vTargetLocalOBBs) {  // itobb is in targetlink coordinates
                OBB cameraobb = geometry::TransformOBB(tCameraInTargetinv,*itobb);
                // SampleProjectedOBBWithTest usually quits when first occlusion is found, so just passing occludingbodyandlinkname to _TestRay should return the initial occluding part.
                if( !SampleProjectedOBBWithTest(cameraobb, _vf->_fSampleRayDensity, boost::bind(&VisibilityConstraintFunction::_TestRay, this, _1, boost::ref(tworldcamera), boost::ref(occludingbodyandlinkname)), tworldcamera, _vf->_fAllowableOcclusion) ) {
                    RAVELOG_VERBOSE("box is occluded\n");
                    errormsg = str(boost::format("{\"type\":\"pattern_occluded\", \"bodylinkname\":\"%s\"}")%occludingbodyandlinkname);
                    return true;
                }
            }
            return false;
        }

        /// check if just the rigidly attached links of the gripper are in the way
        /// this function is not meant to be called during planning (only database generation)
        /// \param tcamera transformation of the camera in the targetlink frame
        bool IsOccludedByRigid(const TransformMatrix& tcamera)
        {
            KinBody::KinBodyStateSaver saver1(_ptargetbox), saver2(_vf->_targetlink->GetParent());
            vector<KinBody::LinkPtr> vattachedlinks;
            _vf->_psensor->GetAttachingLink()->GetRigidlyAttachedLinks(vattachedlinks);
            RobotBase::RobotStateSaver robotsaver(_vf->_robot,RobotBase::Save_LinkTransformation|RobotBase::Save_LinkEnable);
            Transform tsensorinv = _vf->_psensor->GetTransform().inverse();
            // Disable all robot links that are not attached to the camera
            FOREACHC(itlink,_vf->_robot->GetLinks()) {
                bool battached = find(vattachedlinks.begin(),vattachedlinks.end(),*itlink)!=vattachedlinks.end();
                (*itlink)->Enable(battached);
                if( battached ) {
                    (*itlink)->SetTransform(tsensorinv*(*itlink)->GetTransform());
                }
            }
            TransformMatrix tcamerainv = tcamera.inverse();
            Transform ttarget = _vf->_targetlink->GetTransform();
            _ptargetbox->SetTransform(ttarget);
            Transform tworldcamera = ttarget*tcamera;
            _ptargetbox->Enable(true);
            SampleRaysScope srs(*this);
            FOREACH(itobb,_vTargetLocalOBBs) {
                OBB cameraobb = geometry::TransformOBB(tcamerainv,*itobb);
                if( !SampleProjectedOBBWithTest(cameraobb, _vf->_fSampleRayDensity, boost::bind(&VisibilityConstraintFunction::_TestRayRigid, this, _1, boost::ref(tworldcamera),boost::ref(vattachedlinks)), tworldcamera, 0.0f) ) {
                    return true;
                }
            }
            return false;
        }

private:
        /// \brief return true if not occluded by any other target (ray hits the intended target box)
        ///
        /// \param v is in camera coordinate system
        /// \param tcamera is the camera in the world coordinate system
        bool _TestRay(const Vector& v, const TransformMatrix& tcamera, std::string& errormsg)
        {
            RAY r;
            dReal filen = 1/RaveSqrt(v.lengthsqr3());
            r.dir = tcamera.rotate((2.0f*filen)*v);
            r.pos = tcamera.trans + 0.5f*_vf->_fRayMinDist*r.dir;         // move the rays a little forward
            if( !_vf->_robot->GetEnv()->CheckCollision(r,_report) ) {
                return true;         // not supposed to happen, but it is OK
            }

            //            RaveVector<float> vpoints[2];
            //            vpoints[0] = r.pos;
            //            vpoints[1] = _report.contacts[0].pos;
            //            _vf->_robot->GetEnv()->drawlinestrip(vpoints[0],2,16,1.0f,Vector(0,0,1));
            if( !(!!_report->plink1 &&( _report->plink1->GetParent() == _ptargetbox) ) ) {
                if( _report->contacts.size() > 0 ) {
                    Vector v = _report->contacts.at(0).pos;
                    RAVELOG_VERBOSE_FORMAT("bad collision: %s: %f %f %f", _report->__str__()%v.x%v.y%v.z);
                }
                else {
                    RAVELOG_VERBOSE_FORMAT("bad collision: %s", _report->__str__());
                }
            }
            if( !!_report->plink1 ) {
                if( _report->plink1->GetParent() == _ptargetbox ) {
                    // colliding with intended target box, so not being occluded
                    return true;
                }
                else if( _report->plink1 == _vf->_targetlink ) {
                    // the original link is returned, have to check if the collision point is within _ptargetbox since we could be targeting one specific geometry rather than others.
                    if( _report->contacts.size() > 0 ) {
                        // transform the contact point into the target link coordinate system
                        Transform ttarget = _vf->_targetlink->GetTransform();
                        Vector vintargetlink = ttarget.inverse()*_report->contacts.at(0).pos;
                        // if vertex is inside any of the OBBs, then return true. Note: assumes that the original geometries are a box
                        bool bInside = false;
                        FOREACH(itobb, _vTargetLocalOBBs) {
                            Vector voffset = vintargetlink - itobb->pos;
                            if( RaveFabs(voffset.dot3(itobb->right)) <= itobb->extents.x && RaveFabs(voffset.dot3(itobb->up)) <= itobb->extents.y && RaveFabs(voffset.dot3(itobb->dir)) <= itobb->extents.z ) {
                                bInside = true;
                                break;
                            }
                        }

                        return bInside;
                    }
                    else {
                        RAVELOG_WARN("contact link is the target link, however the count of contacting points is 0.");
                        errormsg = "nocontacts";
                        return false;
                    }
                }
                else{
                    std::string linkname = _report->plink1->GetName();
                    std::string bodyname = _report->plink1->GetParent()->GetName();
                    errormsg = bodyname + "/" + linkname;
                    RAVELOG_VERBOSE_FORMAT("Ray hit a non-target body and link named %s, reject.", errormsg);
                    return false;
                }
            }
            else {
                RAVELOG_WARN("ray did not return any contacts, so have to reject\n");
                errormsg = "norayintersection";
                return false;
            }
        }

        bool _TestRayRigid(const Vector& v, const TransformMatrix& tcamera, const vector<KinBody::LinkPtr>& vattachedlinks)
        {
            dReal filen = 1/RaveSqrt(v.lengthsqr3());
            RAY r((_vf->_fRayMinDist*filen)*v,(2.0f*filen)*v);
            if( _vf->_robot->GetEnv()->CheckCollision(r,KinBodyConstPtr(_vf->_robot),_report) ) {
                //RAVELOG_INFO(str(boost::format("ray col: %s\n")%_report->__str__()));
                return false;
            }
            return true;
        }

        CollisionAction _IgnoreCollisionCallback(CollisionReportPtr preport, bool IsCalledFromPhysicsEngine)
        {
            if( _bSamplingRays ) {
                if( !!preport->plink1 ) {
                    if(  !preport->plink1->IsVisible() || preport->plink1->GetParent() == _vf->_sensorrobot ) {
                        return CA_Ignore;
                    }
                }
                if( !!preport->plink2 ) {
                    if( !preport->plink2->IsVisible() || preport->plink2->GetParent() == _vf->_sensorrobot ) {
                        return CA_Ignore;
                    }
                }
            }
            return CA_DefaultAction;
        }
        bool _bSamplingRays;
        boost::shared_ptr<VisualFeedback> _vf;
        KinBodyPtr _ptargetbox;         ///< box to represent the target for simulating ray collisions
        UserDataPtr _collisionfn;

        vector<OBB> _vTargetLocalOBBs;         ///< target geometry bounding boxes in the target link coordinate system
        vector<dReal> _vsolution;
        IkReturnPtr _ikreturn;
        CollisionReportPtr _report;
        AABB _abTarget;         // local aabb in the targetlink coordinate system
        vector<Vector> _vconvexplanes3d; ///< the convex planes of the camera in the target link coordinate system
    };

    class GoalSampleFunction
    {
public:
        GoalSampleFunction(boost::shared_ptr<VisualFeedback> vf, const vector<Transform>& visibilitytransforms) : _vconstraint(vf), _fSampleGoalProb(1.0f), _vf(vf), _visibilitytransforms(visibilitytransforms)
        {
            RAVELOG_DEBUG(str(boost::format("have %d detection extents hypotheses\n")%_visibilitytransforms.size()));
            _ttarget = _vf->_targetlink->GetTransform();
            _sphereperms.PermuteStart(_visibilitytransforms.size());
        }
        virtual ~GoalSampleFunction() {
        }

        virtual bool Sample(vector<dReal>& pNewSample)
        {
            if( RaveRandomFloat() > _fSampleGoalProb ) {
                return false;
            }
            RobotBase::RobotStateSaver state(_vf->_robot);
            std::string errormsg;
            _sphereperms._fn = boost::bind(&GoalSampleFunction::SampleWithParameters,this,_1,boost::ref(pNewSample), false, errormsg);
            if( _sphereperms.PermuteContinue() >= 0 ) {
                return true;
            }
            //            // start from the beginning, if nothing, throw
            //            _sphereperms.PermuteStart(_visibilitytransforms.size());
            //            if( _sphereperms.PermuteContinue() >= 0 )
            //                return true;

            return false;
        }

        bool SampleWithParameters(int isample, vector<dReal>& pNewSample, bool bOutputError, std::string& errormsg)
        {
            TransformMatrix tcamera = _ttarget*_visibilitytransforms.at(isample);
            return _vconstraint.SampleWithCamera(tcamera,pNewSample, bOutputError, errormsg);
        }

        VisibilityConstraintFunction _vconstraint;
        dReal _fSampleGoalProb;
private:
        boost::shared_ptr<VisualFeedback> _vf;
        const vector<Transform>& _visibilitytransforms;


        Transform _ttarget;         ///< transform of target
        RandomPermutationExecutor _sphereperms;
        vector<Transform> _vcameras;         ///< camera transformations in local coord systems
    };

    VisualFeedback(EnvironmentBasePtr penv) : ModuleBase(penv), _preport(new CollisionReport())
    {
        __description = ":Interface Author: Rosen Diankov\n\n\
.. image:: ../../../images/interface_visualfeedback.jpg\n\
  :width: 500\n\n\
Adds grasp planning taking into account camera visibility constraints. The relevant paper is:\n\n\
- Rosen Diankov, Takeo Kanade, James Kuffner. Integrating Grasp Planning and Visual Feedback for Reliable Manipulation, IEEE-RAS Intl. Conf. on Humanoid Robots, December 2009.\n\
\n\
Visibility computation checks occlusion with other objects using ray sampling in the image space:\n\n\
.. image:: ../../../images/interface_visualfeedback_occlusions.jpg\n\
  :height: 200\n\
";
        _fMaxVelMult=1;
        _bIgnoreSensorCollision = false;
        _bCameraOnManip = false;
        _fSampleRayDensity = 0.001;
        _fAllowableOcclusion = 0.1;
        _fRayMinDist = 0.02f;

        RegisterCommand("SetCameraAndTarget",boost::bind(&VisualFeedback::SetCameraAndTarget,this,_1,_2),
                        "Sets the camera index from the robot and its convex hull");
        RegisterCommand("ProcessVisibilityExtents",boost::bind(&VisualFeedback::ProcessVisibilityExtents,this,_1,_2),
                        "Processes the visibility extents of the target and initializes the camera transforms.\n\
\n\
:param sphere: Sets the transforms along a sphere density and the distances\n\
:param conedirangle: Prunes the currently set transforms along a cone centered at the local target center and directed towards conedirangle with a half-angle of ``|conedirangle|``. Can specify multiple cones for an OR effect. The cone represents the visibility of the pattern, should not represent the field of view of the camera.");
        RegisterCommand("SetCameraTransforms",boost::bind(&VisualFeedback::SetCameraTransforms,this,_1,_2),
                        "Sets new camera transformations. Can optionally choose a minimum distance from all planes of the camera convex hull (includes gripper mask)");
        RegisterCommand("ComputeVisibility",boost::bind(&VisualFeedback::ComputeVisibility,this,_1,_2),
                        "Computes the visibility of the current robot configuration");
        RegisterCommand("ComputeVisibleConfiguration",boost::bind(&VisualFeedback::ComputeVisibleConfiguration,this,_1,_2),
                        "Gives a camera transformation, computes the visibility of the object and returns the robot configuration that takes the camera to its specified position, otherwise returns false");
        RegisterCommand("SampleVisibilityGoal",boost::bind(&VisualFeedback::SampleVisibilityGoal,this,_1,_2),
                        "Samples a goal with the current manipulator maintaining camera visibility constraints");
        RegisterCommand("MoveToObserveTarget",boost::bind(&VisualFeedback::MoveToObserveTarget,this,_1,_2),
                        "Approaches a target object while choosing a goal such that the robot's camera sensor sees the object ");
        RegisterCommand("VisualFeedbackGrasping",boost::bind(&VisualFeedback::VisualFeedbackGrasping,this,_1,_2),
                        "Stochastic greedy grasp planner considering visibility");
        RegisterCommand("SetParameter",boost::bind(&VisualFeedback::SetParameter,this,_1,_2),
                        "Sets internal parameters of visibility computation");
    }

    virtual ~VisualFeedback() {
    }

    void Destroy()
    {
        _robot.reset();
        _sensorrobot.reset();
        _targetlink.reset();
        _targetGeomName.clear();
        _psensor.reset();
        _pmanip.reset();
        _pcamerageom.reset();
        _visibilitytransforms.clear();
        _preport.reset();
        ModuleBase::Destroy();
    }

    int main(const string& args)
    {
        stringstream ss(args);
        string robotname;
        _fMaxVelMult=1;
        ss >> robotname;
        _bIgnoreSensorCollision = false;
        string cmd;
        while(!ss.eof()) {
            ss >> cmd;
            if( !ss )
                break;
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
            if( cmd == "maxvelmult" ) {
                ss >> _fMaxVelMult;
            }
            else if( cmd == "ignoresensorcollision" ) {
                ss >> _bIgnoreSensorCollision;
            }
            if( ss.fail() || !ss ) {
                break;
            }
        }
        _robot = GetEnv()->GetRobot(robotname);
        return 0;
    }

    virtual bool SendCommand(std::ostream& sout, std::istream& sinput)
    {
        EnvironmentMutex::scoped_lock lock(GetEnv()->GetMutex());
        return ModuleBase::SendCommand(sout,sinput);
    }

    bool SetCameraAndTarget(ostream& sout, istream& sinput)
    {
        _bCameraOnManip = false;
        _pmanip.reset();
        _psensor.reset();
        _vconvexplanes.resize(0);
        _pcamerageom.reset();
        _targetlink.reset();
        _targetGeomName.clear();
        RobotBase::AttachedSensorPtr psensor;
        RobotBase::ManipulatorPtr pmanip;
        _sensorrobot = _robot;
        bool bHasRayDensity = false;
        string cmd;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput ) {
                break;
            }
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if( cmd == "sensorrobot" ) {
                string name;
                sinput >> name;
                _sensorrobot = GetEnv()->GetRobot(name);
            }
            else if( cmd == "sensorindex" ) {
                int sensorindex=-1;
                sinput >> sensorindex;
                psensor = _sensorrobot->GetAttachedSensors().at(sensorindex);
            }
            else if( cmd == "sensorname" ) {
                string sensorname;
                sinput >> sensorname;
                FOREACH(itsensor,_sensorrobot->GetAttachedSensors()) {
                    if( (*itsensor)->GetName() == sensorname ) {
                        psensor = *itsensor;
                        break;
                    }
                }
            }
            else if( cmd == "targetlink" ) {
                string targetname, targetlinkname;
                sinput >> targetname >> targetlinkname;
                KinBodyPtr ptarget = GetEnv()->GetKinBody(targetname);
                if( !ptarget ) {
                    throw OPENRAVE_EXCEPTION_FORMAT("could not find target %s", targetname, ORE_InvalidArguments);
                }

                _targetlink = ptarget->GetLink(targetlinkname);
                if( !_targetlink ) {
                    throw OPENRAVE_EXCEPTION_FORMAT("could not find target link %s of target %s", targetname%targetlinkname, ORE_InvalidArguments);
                }
            }
            else if( cmd == "targetgeomname" ) {
                sinput >> _targetGeomName;
            }
            else if( cmd == "manipname" ) {
                string manipname;
                sinput >> manipname;
                FOREACH(itmanip,_robot->GetManipulators()) {
                    if( (*itmanip)->GetName() == manipname ) {
                        pmanip = *itmanip;
                        break;
                    }
                }
            }
            else if( cmd == "convexdata" ) {
                int numpoints=0;
                sinput >> numpoints;
                vector<dReal> vconvexdata(2*numpoints);
                FOREACH(it, vconvexdata) {
                    sinput >> *it;
                }
                BOOST_ASSERT(vconvexdata.size() > 2 );
                vector<Vector> vpoints;
                Vector vprev,vprev2,v,vdir,vnorm,vcenter;
                for(size_t i = 0; i < vconvexdata.size(); i += 2 ) {
                    vpoints.push_back(Vector((vconvexdata[i]-_pcamerageom->KK.cx)/_pcamerageom->KK.fx,(vconvexdata[i+1]-_pcamerageom->KK.cy)/_pcamerageom->KK.fy,0,0));
                    vcenter += vpoints.back();
                }

                if( vpoints.size() > 2 ) {
                    vcenter *= 1.0f/vpoints.size();

                    // get the planes
                    _vconvexplanes.reserve(vpoints.size());
                    vprev = vpoints.back();
                    FOREACH(itv, vpoints) {
                        vdir = *itv-vprev;
                        vnorm.x = vdir.y;
                        vnorm.y = -vdir.x;
                        // normal has to be facing inside
                        if( vnorm.x*(vcenter.x-vprev.x)+vnorm.y*(vcenter.y-vprev.y) < 0 ) {
                            vnorm = -vnorm;
                        }
                        vnorm.z = -(vnorm.x*vprev.x+vnorm.y*vprev.y);
                        if( vnorm.lengthsqr3() > 1e-10 ) {
                            _vconvexplanes.push_back(vnorm.normalize3());
                        }
                        vprev = *itv;
                    }

                    // get the center of the convex hull
                    dReal totalarea = 0;
                    _vcenterconvex = Vector(0,0,0);
                    for(size_t i = 2; i < vpoints.size(); ++i) {
                        Vector v0 = vpoints[i-1]-vpoints[0];
                        Vector v1 = vpoints[i]-vpoints[0];
                        dReal area = RaveFabs(v0.x*v1.y-v0.y*v1.x);
                        _vcenterconvex += area*(vpoints[0]+vpoints[i-1]+vpoints[i-1]);
                        totalarea += area;
                    }
                    _vcenterconvex /= 3.0f*totalarea; _vcenterconvex.z = 1;
                }
                else {
                    RAVELOG_WARN(str(boost::format("convex data does not have enough points %d\n")%vconvexdata.size()));
                }
            }
            else if( cmd == "raydensity" ) {
                sinput >> _fSampleRayDensity;
                bHasRayDensity = true;
            }
            else {
                RAVELOG_WARN(str(boost::format("unrecognized command: %s\n")%cmd));
                break;
            }

            if( !sinput ) {
                RAVELOG_ERROR(str(boost::format("failed processing command %s\n")%cmd));
                return false;
            }
        }

        if( !psensor ) {
            return false;
        }

        if( !psensor->GetSensor() ) {
            RAVELOG_WARN(str(boost::format("attached sensor %s does not have sensor interface\n")%psensor->GetName()));
            return false;
        }

        if( psensor->GetSensor()->GetSensorGeometry()->GetType() != SensorBase::ST_Camera) {
            RAVELOG_WARN(str(boost::format("attached sensor %s is not a camera")%psensor->GetName()));
            return false;
        }

        // check if there is a manipulator with the same end effector as camera
        if( _sensorrobot == _robot ) {
            std::vector<KinBody::LinkPtr> vattachedlinks;
            psensor->GetAttachingLink()->GetRigidlyAttachedLinks(vattachedlinks);
            if( !!pmanip ) {
                if(( pmanip->GetEndEffector() != psensor->GetAttachingLink()) &&( find(vattachedlinks.begin(),vattachedlinks.end(),pmanip->GetEndEffector()) == vattachedlinks.end()) ) {
                    RAVELOG_DEBUG(str(boost::format("specified manipulator %s end effector not attached to specified sensor %s\n")%pmanip->GetName()%psensor->GetName()));
                }
                else {
                    _bCameraOnManip = true;
                }
            }
            else {
                FOREACHC(itmanip,_robot->GetManipulators()) {
                    if(( (*itmanip)->GetEndEffector() == psensor->GetAttachingLink()) ||( find(vattachedlinks.begin(),vattachedlinks.end(),(*itmanip)->GetEndEffector()) != vattachedlinks.end()) ) {
                        pmanip = *itmanip;
                        _bCameraOnManip = true;
                        break;
                    }
                }

                if( !pmanip ) {
                    RAVELOG_WARN(str(boost::format("failed to find manipulator with end effector rigidly attached to sensor %s.\n")%psensor->GetName()));
                    pmanip = _robot->GetActiveManipulator();
                }
            }

            _tToManip = psensor->GetTransform().inverse()*pmanip->GetTransform();
        }
        else {
            if( !pmanip ) {
                pmanip = _robot->GetActiveManipulator();
                RAVELOG_INFO_FORMAT("using default manip %s", pmanip->GetName());
            }

            if( !!_targetlink ) {
                _tToManip = _targetlink->GetTransform().inverse() * pmanip->GetTransform();
            }
        }

        _pcamerageom = boost::static_pointer_cast<SensorBase::CameraGeomData const>(psensor->GetSensor()->GetSensorGeometry());
        if( !bHasRayDensity ) {
            _fSampleRayDensity = 20.0f/_pcamerageom->KK.fx;
        }

        if( _vconvexplanes.size() == 0 ) {
            // pick the camera boundaries
            _vconvexplanes.push_back(Vector(_pcamerageom->KK.fx,0,_pcamerageom->KK.cx,0).normalize3());     // -x
            _vconvexplanes.push_back(Vector(-_pcamerageom->KK.fx,0,_pcamerageom->width-_pcamerageom->KK.cx,0).normalize3());     // +x
            _vconvexplanes.push_back(Vector(0,_pcamerageom->KK.fy,_pcamerageom->KK.cy,0).normalize3());     // -y
            _vconvexplanes.push_back(Vector(0,-_pcamerageom->KK.fy,_pcamerageom->height-_pcamerageom->KK.cy,0).normalize3());     // +y
            _vcenterconvex = Vector(0,0,1);
        }

        _pmanip = pmanip;
        _psensor = psensor;
        sout << _pmanip->GetName();
        return true;
    }

    /// \brief Suppose vdir is a direction from the center of a sphere to the surface of the sphere and fdist is its
    ///        radius. This function returns a transformation such that its origin is on the sphere's surface and its
    ///        local Z-axis is pointing radially inwards.
    ///
    /// \param vdir is a unit vector describing the direction.
    /// \param fdist is the translation along vdir
    /// \param froll is the angle for which the local x-and y- axes are rotated.
    /// \param ftilt is the angle to rotate about the axis specified by iaxis
    /// \param iaxis if 0, the transform is rotated about its local X axis. if 1, the transform is rotated about its local Y axis
    TransformMatrix ComputeCameraMatrix(const Vector& vdir,dReal fdist, dReal froll, dReal ftilt=0, int iaxis=0)
    {
        // Compute orthogonal axes from vdir
        Vector vright, vup = Vector(0,1,0) - vdir * vdir.y;
        dReal uplen = vup.lengthsqr3();
        if( uplen < 0.001 ) {
            vup = Vector(0,0,1) - vdir * vdir.z;
            uplen = vup.lengthsqr3();
        }

        vup *= (dReal)1.0/RaveSqrt(uplen);
        vright = vup.cross(vdir);

        // Construct a transformation matrix
        TransformMatrix tcamera;
        dReal fcosroll = RaveCos(froll), fsinroll = RaveSin(froll);
        dReal fcos = RaveCos(ftilt), fsin = RaveSin(ftilt);
        // Rotate around its local Z-axis
        tcamera.m[2] = vdir.x; tcamera.m[6] = vdir.y; tcamera.m[10] = vdir.z;
        tcamera.m[0] = vright.x*fcosroll+vup.x*fsinroll; tcamera.m[1] = -vright.x*fsinroll+vup.x*fcosroll;
        tcamera.m[4] = vright.y*fcosroll+vup.y*fsinroll; tcamera.m[5] = -vright.y*fsinroll+vup.y*fcosroll;
        tcamera.m[8] = vright.z*fcosroll+vup.z*fsinroll; tcamera.m[9] = -vright.z*fsinroll+vup.z*fcosroll;
        tcamera.trans = -fdist * tcamera.rotate(_vcenterconvex); // translation component has to be computed before rotating around the local X

        if( iaxis == 0 ) {
            // Rotate around its local X-axis
            dReal t1 = tcamera.m[1], t5 = tcamera.m[5], t9 = tcamera.m[9];
            dReal t2 = tcamera.m[2], t6 = tcamera.m[6], t10 = tcamera.m[10];
            tcamera.m[1] = fcos*t1 + fsin*t2;  tcamera.m[2]  = -fsin*t1 + fcos*t2;
            tcamera.m[5] = fcos*t5 + fsin*t6;  tcamera.m[6]  = -fsin*t5 + fcos*t6;
            tcamera.m[9] = fcos*t9 + fsin*t10; tcamera.m[10] = -fsin*t9 + fcos*t10;
        }
        else if( iaxis == 1 ) {
            // Rotate around its local Y-axis
            dReal t0 = tcamera.m[0], t4 = tcamera.m[4], t8 = tcamera.m[8];
            dReal t2 = tcamera.m[2], t6 = tcamera.m[6], t10 = tcamera.m[10];
            tcamera.m[0] = fcos*t0 - fsin*t2;  tcamera.m[2]  = fsin*t0 + fcos*t2;
            tcamera.m[4] = fcos*t4 - fsin*t6;  tcamera.m[6]  = fsin*t4 + fcos*t6;
            tcamera.m[8] = fcos*t8 - fsin*t10; tcamera.m[10] = fsin*t8 + fcos*t10;
        }
        return tcamera;
    }

    /// \brief return a list of transforms of the camera in the target link frame.
    bool ProcessVisibilityExtents(ostream& sout, istream& sinput)
    {
        string cmd;
        int numrolls=8;
        int iStartTiltAngle=10, iEndTiltAngle=20, iStepTiltAngle=10; // in degrees
        dReal fToRadian = PI/(dReal) 180;
        vector<Vector> vconedirangles;
        //vector<Transform> vtransforms; // the camera transforms with respect to the target link
        std::vector<Transform> vCamerasInPatternCoord; // cameras with respect to the pattern coordinate system (usually the pattern face is in +z)
        std::vector<Transform> vCamerasInTargetLinkCoord; // cameras with respect to the target link coordinate system. A pattern can be anywhere in the link and depends on geometry transform and cone dir angle.
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput ) {
                break;
            }
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if( cmd == "transforms" ) {
                int numtrans=0;
                sinput >> numtrans;
                vCamerasInPatternCoord.resize(numtrans);
                FOREACH(it,vCamerasInPatternCoord) {
                    sinput >> *it;
                }
            }
            else if( cmd == "numrolls" )
                sinput >> numrolls;
            else if( cmd == "tiltangles" ) {
                sinput >> iStartTiltAngle >> iEndTiltAngle >> iStepTiltAngle;
            }
            else if( cmd == "extents" ) {
                // Each extent is a 3D vector describing the direction from the pattern to the camera. Its norm is the
                // distance. Note that numrolls has to be defined *before* extents.
                int numtrans=0;
                sinput >> numtrans;
                dReal deltaroll = PI*2.0f/(dReal)numrolls;
                vCamerasInPatternCoord.resize(0); vCamerasInPatternCoord.reserve(numtrans*numrolls);
                for(int i = 0; i < numtrans; ++i) {
                    Vector v;
                    sinput >> v.x >> v.y >> v.z;
                    dReal fdist = RaveSqrt(v.lengthsqr3());
                    v *= 1/fdist;
                    dReal froll = 0;
                    for(int iroll = 0; iroll < numrolls; ++iroll, froll += deltaroll) {
                        Transform t = ComputeCameraMatrix(v,fdist,froll);
                        vCamerasInPatternCoord.push_back(t);
                    }
                }
            }
            else if( cmd == "sphere" ) {
                // In the current system we always have the pattern on the
                // robot, so we always use invertsphere. The sphere is first
                // generated in the pattern frame (the calibboard geometry and
                // the cone direction). Camera poses surround the pattern.
                // Invertsphere produces pattern poses around the camera. After
                // inversion, only pattern poses that are in a cone around the
                // (0,0,1) axis of the camera (i.e., in front of the lens) will
                // pass the InConvexHull() check. After filtering out these
                // valid poses, we express them in the targetlink frame and
                // pass them to the IK solver.
                TriMesh spheremesh;
                int spherelevel = 3, numdists = 0;
                sinput >> spherelevel >> numdists;
#ifdef CALIBRATION_TIMING_DEBUG
                uint64_t starttime_spheremesh = utils::GetMicroTime();
#endif
                CM::GenerateSphereTriangulation(spheremesh,spherelevel);
#ifdef CALIBRATION_TIMING_DEBUG
                uint64_t endtime_spheremesh = utils::GetMicroTime();
                float fElapsed_spheremesh = (endtime_spheremesh - starttime_spheremesh)*1e-6f;
                RAVELOG_DEBUG_FORMAT("sphere mesh generation: %f s.", fElapsed_spheremesh);
#endif
                if( 0 ) {
                    std::stringstream ss; ss << std::setprecision(std::numeric_limits<dReal>::digits10+1);
                    ss << "vertices=[";
                    FOREACHC(itvertex, spheremesh.vertices) {
                        ss << "[" << itvertex->x << "," << itvertex->y << "," << itvertex->z << "],";
                    }
                    ss << "]; indices=[";
                    FOREACHC(itindex, spheremesh.indices) {
                        ss << *itindex << ",";
                    }
                    ss << "];";
                    RAVELOG_DEBUG_FORMAT("%s", ss.str());
                }
                vector<dReal> vdists(numdists);
                FOREACH(it,vdists) {
                    sinput >> *it;
                }
                bool bInvert = _sensorrobot != _robot; // if True, then invert sampled transforms of the sphere.
                dReal deltaroll = PI*2.0f/(dReal)numrolls;
                dReal deltatilt = iStepTiltAngle * fToRadian;
                int numtilts = ((iEndTiltAngle - iStartTiltAngle)/iStepTiltAngle) + 1;
                int numaxes = 2;
                vCamerasInPatternCoord.resize(spheremesh.vertices.size()*numdists*numrolls*numtilts*numaxes*2);
                vector<Transform>::iterator itcamera = vCamerasInPatternCoord.begin();
#ifdef CALIBRATION_TIMING_DEBUG
                uint64_t starttime_computetransform = utils::GetMicroTime();
#endif
                for(size_t j = 0; j < spheremesh.vertices.size(); ++j) {
                    Vector v = spheremesh.vertices[j];
                    for(int i = 0; i < numdists; ++i) {
                        dReal froll = 0;
                        for(int iroll = 0; iroll < numrolls; ++iroll, froll += deltaroll) {
                            for(int iaxis = 0; iaxis < numaxes; ++iaxis) {
                                // Consider both local X and Y rotation
                                dReal ftilt = iStartTiltAngle * fToRadian;
                                for(int itilt = 0; itilt < numtilts; ++itilt, ftilt += deltatilt) {
                                    // Consider positive tilting angle
                                    *itcamera = ComputeCameraMatrix(spheremesh.vertices[j], vdists[i], froll, ftilt, iaxis);
                                    if( bInvert ) {
                                        *itcamera = itcamera->inverse();
                                    }
                                    ++itcamera;

                                    // Consider negative tilting angle
                                    *itcamera = ComputeCameraMatrix(spheremesh.vertices[j], vdists[i], froll, -ftilt, iaxis);
                                    if( bInvert ) {
                                        *itcamera = itcamera->inverse();
                                    }
                                    ++itcamera;
                                }
                            }
                        }
                    }
                }
#ifdef CALIBRATION_TIMING_DEBUG
                uint64_t endtime_computetransform = utils::GetMicroTime();
                float fElapsed_computetransform = (endtime_computetransform - starttime_computetransform)*1e-6f;
                RAVELOG_DEBUG_FORMAT("transform generation (total=%d): %f s.", vCamerasInPatternCoord.size()%fElapsed_computetransform);
#endif
            }
            else if( cmd == "conedirangle" ) {
                // conedirangle includes both cone direction and angle info. The
                // magnitude of conedirangle encodes the angle (i.e., radius) of the
                // cone.
                Vector vconedir; dReal fangle;
                sinput >> vconedir.x >> vconedir.y >> vconedir.z;
                fangle = RaveSqrt(vconedir.lengthsqr3());
                if( fangle == 0 ) {
                    return false;
                }

                vconedir /= fangle;
                vconedir.w = RaveCos(fangle);
                vconedirangles.push_back(vconedir);
            }
            else {
                RAVELOG_WARN(str(boost::format("unrecognized command: %s\n")%cmd));
                break;
            }

            if( !sinput ) {
                RAVELOG_ERROR(str(boost::format("failed processing command %s\n")%cmd));
                return false;
            }
        }

        if( vconedirangles.size() > 0 ) {
            vCamerasInTargetLinkCoord.reserve(vCamerasInPatternCoord.size());
            Transform tGeometryToTargetLink;
            for(size_t igeom = 0; igeom < _targetlink->GetGeometries().size(); ++igeom) {
                if( _targetGeomName.size() == 0 || _targetlink->GetGeometries().at(igeom)->GetName() == _targetGeomName ) {
                    tGeometryToTargetLink = _targetlink->GetGeometries().at(igeom)->GetTransform();
                    break;
                }
            }

            dReal fCameraCosFOV = 0; // cos of the half angle of the camera FOV, in this case cos(pi/2)

#ifdef CALIBRATION_TIMING_DEBUG
            uint64_t starttime_filtertransforms = utils::GetMicroTime();
#endif
            Vector pluszvector;
            pluszvector.x = 0;
            pluszvector.y = 0;
            pluszvector.z = 1;
            FOREACH(itt,vCamerasInPatternCoord) {
                Vector vCameraDirInPattern = ExtractAxisFromQuat(itt->rot, 2); // camera is always +z
                Vector vCameraToPatternPos = itt->trans;
                dReal flen = RaveSqrt(vCameraToPatternPos.lengthsqr3());
                if( -vCameraToPatternPos.dot3(vCameraDirInPattern) < fCameraCosFOV * flen ) {
                    // outside of camera FOV
                    continue;
                }

                bool bInCone = false;

                // We hard-code the cone direction (0,0,1) for initial visibility checking
                // because only patterns in front of the camera lens will pass InConvexHull().
                // We then appropriately express these pattern poses in the targetlink
                // frame, taking into account vconedirangles.
                FOREACH(itcone, vconedirangles) {
                    if( pluszvector.dot3(vCameraToPatternPos) >= itcone->w*flen ) {
                        // quatRotationDirection uses only the first three values of the vectors
                        Vector vConeQuat = geometry::quatRotateDirection(pluszvector, *itcone);
                        Transform vPatternToGeometry = geometry::matrixFromQuat(vConeQuat);
                        Transform vCameraToTargetLink = tGeometryToTargetLink * vPatternToGeometry * *itt;
                        vCamerasInTargetLinkCoord.push_back(vCameraToTargetLink);
                    }
                }
            }
#ifdef CALIBRATION_TIMING_DEBUG
            uint64_t endtime_filtertransforms = utils::GetMicroTime();
            float fElapsed_filtertransforms = (endtime_filtertransforms - starttime_filtertransforms)*1e-6f;
            RAVELOG_DEBUG_FORMAT("transform filtering by cone dirangles (remaining=%d): %f s.", vCamerasInTargetLinkCoord.size()%fElapsed_filtertransforms);
#endif
        }
        else {
            vCamerasInTargetLinkCoord.resize(vCamerasInPatternCoord.size());
            for(size_t i = 0; i < vCamerasInTargetLinkCoord.size(); ++i) {
                vCamerasInTargetLinkCoord[i] = vCamerasInPatternCoord[i];
            }
        }

        KinBody::KinBodyStateSaver saver(_targetlink->GetParent(),KinBody::Save_LinkTransformation);
        _targetlink->SetTransform(Transform()); // not sure why _targetlink has to be set to identity here
        boost::shared_ptr<VisibilityConstraintFunction> pconstraintfn(new VisibilityConstraintFunction(shared_problem()));
        saver.Restore(); // need to restore _targetlink so that the preceeding CheckEndEffectorCollision works correctly

        RAVELOG_DEBUG_FORMAT("targetlink.IsEnabled()=%d", _targetlink->IsEnabled());
#ifdef CALIBRATION_TIMING_DEBUG
        float fElapsed_convexhull = 0;
        uint64_t starttime_convexhull, endtime_convexhull;
        float fElapsed_eecollision = 0;
        uint64_t starttime_eecollision, endtime_eecollision;
        float fElapsed_eeocclusion = 0;
        uint64_t starttime_eeocclusion, endtime_eeocclusion;
#endif
        // get all the camera positions and test them
        FOREACHC(itcamera, vCamerasInTargetLinkCoord) {
            Transform tCameraInTarget = *itcamera;
            Transform tTargetInWorld;
            if( _sensorrobot == _robot ) {
                tTargetInWorld = _sensorrobot->GetTransform() * tCameraInTarget.inverse();
            }
            else {
                tTargetInWorld = _sensorrobot->GetTransform() * tCameraInTarget.inverse();
            }

#ifdef CALIBRATION_TIMING_DEBUG
            starttime_convexhull = utils::GetMicroTime();
#endif
            if( pconstraintfn->InConvexHull(*itcamera) ) {
#ifdef CALIBRATION_TIMING_DEBUG
                endtime_convexhull = utils::GetMicroTime();
                fElapsed_convexhull += (endtime_convexhull - starttime_convexhull)*1e-6f;
                starttime_eecollision = utils::GetMicroTime();
#endif
                if( !_pmanip->CheckEndEffectorCollision(tTargetInWorld*_tToManip, _preport) ) {
#ifdef CALIBRATION_TIMING_DEBUG
                    endtime_eecollision = utils::GetMicroTime();
                    fElapsed_eecollision += (endtime_eecollision - starttime_eecollision)*1e-6f;
                    starttime_eeocclusion = utils::GetMicroTime();
#endif
                    if( !pconstraintfn->IsOccludedByRigid(*itcamera) ) {
#ifdef CALIBRATION_TIMING_DEBUG
                        endtime_eeocclusion = utils::GetMicroTime();
                        fElapsed_eeocclusion += (endtime_eeocclusion - starttime_eeocclusion)*1e-6f;
#endif
                        sout << *itcamera << " ";
                        std::stringstream ss; ss << std::setprecision(std::numeric_limits<dReal>::digits10+1);
                        Transform tManip = tTargetInWorld*_tToManip;
                        ss << "manippose=[" << tManip.rot.x << "," << tManip.rot.y << "," << tManip.rot.z << "," << tManip.rot.w << "," << tManip.trans.x << "," << tManip.trans.y << "," << tManip.trans.z << "];";
                        RAVELOG_VERBOSE_FORMAT("passed all tests: %s", ss.str());
                    }
                    else {
#ifdef CALIBRATION_TIMING_DEBUG
                        endtime_eeocclusion = utils::GetMicroTime();
                        fElapsed_eeocclusion += (endtime_eeocclusion - starttime_eeocclusion)*1e-6f;
#endif
                        RAVELOG_VERBOSE("in convex hull and effector is free, but not occluded by rigid\n");
                    }
                }
                else {
#ifdef CALIBRATION_TIMING_DEBUG
                    endtime_eecollision = utils::GetMicroTime();
                    fElapsed_eecollision += (endtime_eecollision - starttime_eecollision)*1e-6f;
#endif
                    std::stringstream ss; ss << std::setprecision(std::numeric_limits<dReal>::digits10+1);
                    Transform tManip = tTargetInWorld*_tToManip;
                    ss << "manippose=[" << tManip.rot.x << "," << tManip.rot.y << "," << tManip.rot.z << "," << tManip.rot.w << "," << tManip.trans.x << "," << tManip.trans.y << "," << tManip.trans.z << "];";
                    RAVELOG_VERBOSE_FORMAT("in convex hull, but end effector collision: %s; %s", _preport->__str__()%ss.str());
                }
            }
            else {
#ifdef CALIBRATION_TIMING_DEBUG
                endtime_convexhull = utils::GetMicroTime();
                fElapsed_convexhull += (endtime_convexhull - starttime_convexhull)*1e-6f;
#endif
            }
        }

#ifdef CALIBRATION_TIMING_DEBUG
        RAVELOG_DEBUG_FORMAT("Timing results (total transforms checked=%d):\n    convex hull checking: %f s.\n    ee collision checking: %f s.\n    ee occlusion checking: %f s.", vCamerasInTargetLinkCoord.size()%fElapsed_convexhull%fElapsed_eecollision%fElapsed_eeocclusion);
#endif

        return true;
    }

    bool SetCameraTransforms(ostream& sout, istream& sinput)
    {
        string cmd;
        _visibilitytransforms.resize(0);
        dReal mindist = 0;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput ) {
                break;
            }
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if( cmd == "transforms" ) {
                size_t numtrans=0;
                sinput >> numtrans;
                _visibilitytransforms.resize(numtrans);
                Transform t;
                for(size_t i =0; i < numtrans; ++i) {
                    sinput >> _visibilitytransforms[i];
                }
            }
            else if( cmd == "mindist" ) {
                sinput >> mindist;
            }
            else {
                RAVELOG_WARN(str(boost::format("unrecognized command: %s\n")%cmd));
                break;
            }

            if( !sinput ) {
                RAVELOG_ERROR(str(boost::format("failed processing command %s\n")%cmd));
                return false;
            }
        }

        if( mindist != 0 ) {
            KinBody::KinBodyStateSaver saver(_targetlink->GetParent());
            _targetlink->SetTransform(Transform());
            boost::shared_ptr<VisibilityConstraintFunction> pconstraintfn(new VisibilityConstraintFunction(shared_problem()));
            vector<Transform> visibilitytransforms; visibilitytransforms.swap(_visibilitytransforms);
            _visibilitytransforms.reserve(visibilitytransforms.size());
            FOREACH(it,visibilitytransforms) {
                if( pconstraintfn->InConvexHull(*it,mindist) ) {
                    _visibilitytransforms.push_back(*it);
                }
            }
        }

        return true;
    }

    bool ComputeVisibility(ostream& sout, istream& sinput)
    {
        bool bcheckocclusion = true;
        sinput >> bcheckocclusion;

        RobotBase::RobotStateSaver saver(_robot);
        _robot->SetActiveManipulator(_pmanip);
        _robot->SetActiveDOFs(_pmanip->GetArmIndices());
        boost::shared_ptr<VisibilityConstraintFunction> pconstraintfn(new VisibilityConstraintFunction(shared_problem()));

        std::string errormsg;
        sout << pconstraintfn->IsVisible(bcheckocclusion, false, errormsg);
        return true;
    }

    bool SetParameter(ostream& sout, istream& sinput)
    {
        string cmd;
        Transform t;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput ) {
                break;
            }
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if( cmd == "raydensity" ) {
                sinput >> _fSampleRayDensity;
            }
            else if( cmd == "raymindist") {
                sinput >> _fRayMinDist;
            }
            else if( cmd == "allowableocclusion" ) {
                sinput >> _fAllowableOcclusion;
            }
            else {
                RAVELOG_WARN(str(boost::format("unrecognized command: %s\n")%cmd));
                break;
            }

            if( !sinput ) {
                RAVELOG_ERROR(str(boost::format("failed processing command %s\n")%cmd));
                return false;
            }
        }
        return true;
    }

    bool ComputeVisibleConfiguration(ostream& sout, istream& sinput)
    {
        string cmd;
        Transform t;  // transform of targetlink in world coordinate system
        // Therefore t * _tToManip = tmanipinworld.
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput ) {
                break;
            }
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if( cmd == "pose" ) {
                sinput >> t;
            }
            else {
                RAVELOG_WARN(str(boost::format("unrecognized command: %s\n")%cmd));
                break;
            }

            if( !sinput ) {
                RAVELOG_ERROR(str(boost::format("failed processing command %s\n")%cmd));
                return false;
            }
        }

        vector<dReal> vsample;
        RobotBase::RobotStateSaver saver(_robot);
        _robot->SetActiveManipulator(_pmanip);
        _robot->SetActiveDOFs(_pmanip->GetArmIndices());
        boost::shared_ptr<VisibilityConstraintFunction> pconstraintfn(new VisibilityConstraintFunction(shared_problem()));

        if( _pmanip->CheckEndEffectorCollision(t*_tToManip, _preport) ) {
            RAVELOG_VERBOSE_FORMAT("endeffector is in collision, %s\n",_preport->__str__());
            std::string errormsg = _preport->__str__();
            boost::replace_all(errormsg, "\"", "\\\"");
            sout << "{\"error\":{\"type\":\"endeffector\", \"report\":\"" << errormsg << "\"}}";
            return true;
        }
        std::string errormsg;
        if( !pconstraintfn->SampleWithCamera(t,vsample, true, errormsg) ) {
            // TODO have better error message on why this failed!
            //boost::replace_all(errormsg, "\"", "\\\"");   // error message
            //already has escape characters at this point
            sout << "{\"error\":" << errormsg << "}";
            return true;
        }

        // have a sample!
        sout << "{\"solution\":[";
        for(size_t i = 0; i < vsample.size(); ++i) {
            if( i > 0 ) {
                sout << ",";
            }
            sout << vsample[i];
        }
        sout << "]}";
        return true;
    }

    bool SampleVisibilityGoal(ostream& sout, istream& sinput)
    {
        string cmd;

        int numsamples=1;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput ) {
                break;
            }
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if( cmd == "numsamples" ) {
                sinput >> numsamples;
            }
            else {
                RAVELOG_WARN(str(boost::format("unrecognized command: %s\n")%cmd));
                break;
            }

            if( !sinput ) {
                RAVELOG_ERROR(str(boost::format("failed processing command %s\n")%cmd));
                return false;
            }
        }

        RobotBase::RobotStateSaver saver(_robot);
        _robot->SetActiveManipulator(_pmanip);
        _robot->SetActiveDOFs(_pmanip->GetArmIndices());

        if( _pmanip->CheckIndependentCollision(_preport) ) {
            RAVELOG_WARN(str(boost::format("robot independent links in collision: %s\n")%_preport->__str__()));
            return false;
        }

        boost::shared_ptr<GoalSampleFunction> pgoalsampler(new GoalSampleFunction(shared_problem(),_visibilitytransforms));

        uint64_t starttime = utils::GetMicroTime();
        vector<dReal> vsample;
        vector<dReal> vsamples(_robot->GetActiveDOF()*numsamples);
        int numsampled = 0;
        for(int i = 0; i < numsamples; ++i) {
            if( pgoalsampler->Sample(vsample) ) {
                std::copy(vsample.begin(), vsample.end(),vsamples.begin()+i*_robot->GetActiveDOF());
                numsampled++;
            }
        }

        if( numsampled == 0 ) {
            return false;
        }
        float felapsed = (utils::GetMicroTime()-starttime)*1e-6f;
        RAVELOG_INFO("total time for %d samples is %fs, %f avg\n", numsamples,felapsed,felapsed/numsamples);
        sout << numsampled << " ";
        for(int i = 0; i < numsampled*_robot->GetActiveDOF(); ++i) {
            sout << vsamples[i] << " ";
        }
        return true;
    }

    bool MoveToObserveTarget(ostream& sout, istream& sinput)
    {
        string strtrajfilename;
        bool bExecute = true;
        boost::shared_ptr<ostream> pOutputTrajStream;

        PlannerBase::PlannerParametersPtr params(new PlannerBase::PlannerParameters());
        params->_nMaxIterations = 4000;
        int affinedofs=0;
        string cmd, plannername="BiRRT";
        dReal fSampleGoalProb = 0.001f;

        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput ) {
                break;
            }
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if( cmd == "outputtraj" ) {
                pOutputTrajStream = boost::shared_ptr<ostream>(&sout,utils::null_deleter());
            }
            else if( cmd == "affinedofs" ) {
                sinput >> affinedofs;
            }
            else if( cmd == "maxiter" ) {
                sinput >> params->_nMaxIterations;
            }
            else if( cmd == "execute" ) {
                sinput >> bExecute;
            }
            else if( cmd == "writetraj" ) {
                sinput >> strtrajfilename;
            }
            else if( cmd == "smoothpath" ) {
                sinput >> params->_sPostProcessingPlanner;
            }
            else if( cmd == "planner" ) {
                sinput >> plannername;
            }
            else if( cmd == "sampleprob" ) {
                sinput >> fSampleGoalProb;
            }
            else {
                RAVELOG_WARN(str(boost::format("unrecognized command: %s\n")%cmd));
                break;
            }

            if( !sinput ) {
                RAVELOG_ERROR(str(boost::format("failed processing command %s\n")%cmd));
                return false;
            }
        }

        RobotBase::RobotStateSaver saver(_robot);
        _robot->SetActiveManipulator(_pmanip);
        _robot->SetActiveDOFs(_pmanip->GetArmIndices(), affinedofs);

        boost::shared_ptr<GoalSampleFunction> pgoalsampler(new GoalSampleFunction(shared_problem(),_visibilitytransforms));
        pgoalsampler->_fSampleGoalProb = fSampleGoalProb;
        _robot->RegrabAll();

        params->SetRobotActiveJoints(_robot);
        _robot->GetActiveDOFValues(params->vinitialconfig);

        params->_samplegoalfn = boost::bind(&GoalSampleFunction::Sample,pgoalsampler,_1);
        TrajectoryBasePtr ptraj = RaveCreateTrajectory(GetEnv(),"");
        ptraj->Init(_robot->GetActiveConfigurationSpecification());
        ptraj->Insert(0,params->vinitialconfig);

        // jitter for initial collision
        if( !planningutils::JitterActiveDOF(_robot) ) {
            RAVELOG_WARN("jitter failed for initial\n");
            return false;
        }
        _robot->GetActiveDOFValues(params->vinitialconfig);

        PlannerBasePtr planner = RaveCreatePlanner(GetEnv(),plannername);
        if( !planner ) {
            RAVELOG_ERROR("failed to create BiRRTs\n");
            return false;
        }

        bool bSuccess = false;
        RAVELOG_INFOA("starting planning\n");
        uint64_t starttime = utils::GetMicroTime();
        for(int iter = 0; iter < 1; ++iter) {
            if( !planner->InitPlan(_robot, params) ) {
                RAVELOG_ERROR("InitPlan failed\n");
                return false;
            }
            if( planner->PlanPath(ptraj) ) {
                bSuccess = true;
                RAVELOG_INFOA("finished planning\n");
                break;
            }
            else {
                RAVELOG_WARN("PlanPath failed\n");
            }
        }

        float felapsed = (utils::GetMicroTime()-starttime)*0.000001f;
        RAVELOG_INFOA("total planning time: %fs\n", felapsed);
        if( !bSuccess ) {
            return false;
        }
        if( RaveGetDebugLevel() & Level_VerifyPlans ) {
            planningutils::VerifyTrajectory(params,ptraj);
        }
        CM::SetActiveTrajectory(_robot, ptraj, bExecute, strtrajfilename, pOutputTrajStream,_fMaxVelMult);
        return true;
    }

    bool VisualFeedbackGrasping(ostream& sout, istream& sinput)
    {
        string strtrajfilename;
        bool bExecute = true;
        boost::shared_ptr<ostream> pOutputTrajStream;

        boost::shared_ptr<GraspSetParameters> params(new GraspSetParameters(GetEnv()));
        params->_nMaxIterations = 4000;
        string cmd, plannername="GraspGradient";
        params->_fVisibiltyGraspThresh = 0.05f;
        bool bUseVisibility = false;
        while(!sinput.eof()) {
            sinput >> cmd;
            if( !sinput ) {
                break;
            }
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if( cmd == "outputtraj" ) {
                pOutputTrajStream = boost::shared_ptr<ostream>(&sout,utils::null_deleter());
            }
            else if( cmd == "maxiter" ) {
                sinput >> params->_nMaxIterations;
            }
            else if( cmd == "visgraspthresh" ) {
                sinput >> params->_fVisibiltyGraspThresh;
            }
            else if( cmd == "execute" ) {
                sinput >> bExecute;
            }
            else if( cmd == "writetraj" ) {
                sinput >> strtrajfilename;
            }
            else if( cmd == "usevisibility" ) {
                sinput >> bUseVisibility;
            }
            else if( cmd == "planner" ) {
                sinput >> plannername;
            }
            else if( cmd == "graspdistthresh" ) {
                sinput >> params->_fGraspDistThresh;
            }
            else if( cmd == "graspset" ) {
                int numgrasps=0;
                sinput >> numgrasps;
                params->_vgrasps.resize(numgrasps);
                TransformMatrix t;
                FOREACH(itgrasp,params->_vgrasps) {
                    sinput >> t;
                    *itgrasp = t;
                }
                RAVELOG_DEBUG(str(boost::format("grasp set size = %d\n")%params->_vgrasps.size()));
            }
            else if( cmd == "numgradientsamples" ) {
                sinput >> params->_nGradientSamples;
            }
            else {
                RAVELOG_WARN(str(boost::format("unrecognized command: %s\n")%cmd));
                return false;
            }

            if( !sinput ) {
                RAVELOG_ERROR(str(boost::format("failed processing command %s\n")%cmd));
                return false;
            }
        }

        RobotBase::RobotStateSaver saver(_robot);
        _robot->SetActiveManipulator(_pmanip);
        _robot->SetActiveDOFs(_pmanip->GetArmIndices());
        params->SetRobotActiveJoints(_robot);

        if( bUseVisibility ) {
            RAVELOG_DEBUG("using visibility constraints\n");
            boost::shared_ptr<VisibilityConstraintFunction> pconstraint(new VisibilityConstraintFunction(shared_problem()));
            params->_checkpathconstraintsfn = boost::bind(&VisibilityConstraintFunction::Constraint,pconstraint,params->_checkpathconstraintsfn,_1,_2,_3,_4);
        }

        params->_ptarget = _targetlink->GetParent();
        _robot->GetActiveDOFValues(params->vinitialconfig);

        TrajectoryBasePtr ptraj = RaveCreateTrajectory(GetEnv(),"");
        ptraj->Init(_robot->GetActiveConfigurationSpecification());
        PlannerBasePtr planner = RaveCreatePlanner(GetEnv(),plannername);
        if( !planner ) {
            RAVELOG_ERROR("failed to create BiRRTs\n");
            return false;
        }

        bool bSuccess = false;
        RAVELOG_INFOA("starting planning\n");
        uint64_t starttime = utils::GetMicroTime();
        if( !planner->InitPlan(_robot, params) ) {
            RAVELOG_ERROR("InitPlan failed\n");
            return false;
        }

        if( planner->PlanPath(ptraj) ) {
            bSuccess = true;
        }
        else {
            RAVELOG_WARN("PlanPath failed\n");
        }

        float felapsed = (utils::GetMicroTime()-starttime)*0.000001f;
        RAVELOG_INFOA("total planning time: %fs\n", felapsed);
        if( !bSuccess ) {
            return false;
        }
        if( RaveGetDebugLevel() & Level_VerifyPlans ) {
            planningutils::VerifyTrajectory(params,ptraj);
        }
        CM::SetActiveTrajectory(_robot, ptraj, bExecute, strtrajfilename, pOutputTrajStream,_fMaxVelMult);
        return true;
    }

    bool SimulationStep(dReal fElapsedTime)
    {
        return false;
    }

protected:
    RobotBasePtr _robot, _sensorrobot;
    bool _bIgnoreSensorCollision; ///< if true will ignore any collisions with vf->_sensorrobot
    KinBody::LinkPtr _targetlink; ///< the link where the verification pattern is attached
    std::string _targetGeomName; ///< if not empty, then use only a specific geometry of the link rather than all geometries
    dReal _fMaxVelMult;
    RobotBase::AttachedSensorPtr _psensor;
    RobotBase::ManipulatorPtr _pmanip;
    bool _bCameraOnManip;     ///< true if camera is attached to manipulator
    SensorBase::CameraGeomDataConstPtr _pcamerageom;
    Transform _tToManip;     ///< transforms a coord system from the link to the gripper coordsystem. tLinkInWorld * _tToManip = tManipInWorld
    vector<Transform> _visibilitytransforms; ///< the transform with respect to the targetlink and camera (or vice-versa)
    dReal _fRayMinDist, _fAllowableOcclusion, _fSampleRayDensity;

    CollisionReportPtr _preport;

    vector<Vector> _vconvexplanes;     ///< the planes defining the bounding visibility region (posive is inside). Inside camera coordinate system
    Vector _vcenterconvex;     ///< center point on the z=1 plane of the convex region
};

ModuleBasePtr CreateVisualFeedback(EnvironmentBasePtr penv) {
    return ModuleBasePtr(new VisualFeedback(penv));
}
