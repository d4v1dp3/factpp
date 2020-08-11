/* ======================================================================== *\
!
! *
! * This file is part of MARS, the MAGIC Analysis and Reconstruction
! * Software. It is distributed to you in the hope that it can be a useful
! * and timesaving tool in analysing Data of imaging Cerenkov telescopes.
! * It is distributed WITHOUT ANY WARRANTY.
! *
! * Permission to use, copy, modify and distribute this software and its
! * documentation for any purpose is hereby granted without fee,
! * provided that the above copyright notice appear in all copies and
! * that both that copyright notice and this permission notice appear
! * in supporting documentation. It is provided "as is" without express
! * or implied warranty.
! *
!
!
!   Author(s): Thomas Bretz 1/2008 <mailto:thomas.bretz@epfl.ch>
!
!   Copyright: MAGIC Software Development, 2000-2011
!
!
\* ======================================================================== */

/////////////////////////////////////////////////////////////////////////////
//
// MVideo
//
// Interface to Video4Linux at a simple level
//
// V4L2 spcifications from http://v4l2spec.bytesex.org/spec/
//
/////////////////////////////////////////////////////////////////////////////
#include "MVideo.h"

// iostream
#include <iostream>

// open
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>    // usleep
#include <errno.h>     // errno
#include <sys/mman.h>  // mmap
#include <sys/ioctl.h> // ioctl

#include <TEnv.h>
#include <TString.h>

#include "MLog.h"
#include "MLogManip.h"

#undef DEBUG

using namespace std;

//ClassImp(MVideo);

MVideoCtrl::MVideoCtrl(const v4l2_queryctrl &ctrl)
{
    fId      = ctrl.id;
    fName    = (const char*)ctrl.name;
    fMinimum = ctrl.minimum;
    fMaximum = ctrl.maximum;
    fStep    = ctrl.step;
    fDefault = ctrl.default_value;
}

// -------------------------------------------------------------
//
// Constructor. Specify the device (e.g. "/dev/video") to be used
//
MVideo::MVideo(const char *path) : fPath(path), fFileDesc(-1), fMapBuffer(0)
{
    Reset();

    fControls.SetOwner();
}

// -------------------------------------------------------------
//
// Internal function to reset the descriptors of the device
//
void MVideo::Reset()
{
    fInputs.clear();
    fStandards.clear();

    memset(&fCaps,    0, sizeof(fCaps));
    memset(&fChannel, 0, sizeof(fChannel));
//    memset(&fBuffer,  0, sizeof(fBuffer));
    memset(&fAbil,    0, sizeof(fAbil));

    fFileDesc = -1;
    fMapBuffer = 0;
    fChannel.channel = -1;
    fAbil.tuner = -1;

    fControls.Delete();
}

// -------------------------------------------------------------
//
// Mapper around ioctl for easier access to the device
//
int MVideo::Ioctl(int req, void *opt, bool allowirq, bool force) const
{
    if (fFileDesc<0)
    {
        gLog << err << "ERROR - Ioctl: Device " << fPath << " not open." << endl;
        return -1;
    }

    while (1)
    {
        // FIXME: This call is a possible source for a hangup
        const int rc = ioctl(fFileDesc, req, opt);
        if (rc==0)
            return 0;

        if (errno==EINVAL)
            return 1;

        if (!allowirq && errno==EAGAIN)
            return -4;

        cout <<"errno="<< errno << endl;

        // errno== 4: Interrupted system call (e.g. by alarm())
        // errno==16: Device or resource busy
        if (errno==4 || errno==16)
        {
            if (!allowirq && errno==4)
                return -4;

            gLog << err << "ERROR - MVideo::Ioctl 0x" << hex << req << ": errno=" << dec << errno << " - ";
            gLog << strerror(errno) << " (rc=" << rc << ")" << endl;
            usleep(10);
            continue;
        }

        if (!force)
        {
            gLog << err << "ERROR - MVideo::Ioctl 0x" << hex << req << ": errno=" << dec << errno << " - ";
            gLog << strerror(errno) << " (rc=" << rc << ")" << endl;
        }
        return rc;
    }
    return -1;
}

// -------------------------------------------------------------
//
// Read the capabilities of the device
//
Bool_t MVideo::GetCapabilities()
{
    return Ioctl(VIDIOCGCAP, &fCaps)!=-1;
}

// -------------------------------------------------------------
//
// Read the properties of the device
//
Bool_t MVideo::GetProperties()
{
    return Ioctl(VIDIOCGPICT, &fProp)!=-1;
}

// -------------------------------------------------------------
//
// Read the video standard
//
Bool_t MVideo::GetVideoStandard()
{
    return Ioctl(VIDIOC_G_STD, &fVideoStandard)==-1;
}

// -------------------------------------------------------------
//
// Read the abilities of the tuner
//
Bool_t MVideo::GetTunerAbilities()
{
    fAbil.tuner = 0; // FIXME?
    return Ioctl(VIDIOCGTUNER, &fAbil)!=-1;
}

// -------------------------------------------------------------
//
// Enumerate (get) all controls from the device and store them
// as MVideoCtrl in fControls, starting with the id given as
// argument.
//
Bool_t MVideo::EnumerateControls(UInt_t id)
{
    struct v4l2_queryctrl qctrl;
    memset(&qctrl, 0, sizeof(qctrl));
    qctrl.id = id;

    while (1)
    {
        if (Ioctl(VIDIOC_QUERYCTRL, &qctrl, true, true)==-1)
            break;

        if (qctrl.maximum<=qctrl.minimum)
            continue;

        fControls.Add(new MVideoCtrl(qctrl));

        qctrl.id++;
    }

    return kTRUE;
}

// -------------------------------------------------------------
//
// Enumerate (get) all basic and private controls from the
// device and store them as MVideoCtrl in fControls.
//
Bool_t MVideo::EnumerateControls()
{
    if (!EnumerateControls(V4L2_CID_BASE))
        return kFALSE;
    if (!EnumerateControls(V4L2_CID_PRIVATE_BASE))
        return kFALSE;

    return kTRUE;
}

// -------------------------------------------------------------
//
// Reset a given control to it's default value as defined
// by the device.
//
Bool_t MVideo::ResetControl(MVideoCtrl &vctrl) const
{
    return WriteControl(vctrl, vctrl.fDefault);
}

// -------------------------------------------------------------
//
// Reset all enumereated device controls to their default.
// The default is defined by the device iteself.
//
Bool_t MVideo::ResetControls() const
{
    Bool_t rc = kTRUE;

    TIter Next(&fControls);
    MVideoCtrl *ctrl = 0;
    while ((ctrl=((MVideoCtrl*)Next())))
        if (!ResetControl(*ctrl))
        {
            gLog << err << "ERROR - Could not reset " << ctrl->fName << "." << endl;
            rc = kFALSE;
        }

    return rc;
}

// -------------------------------------------------------------
//
//  Read the value of the given control from the device
// and store it back into the given MVideoCtrl.
//
Bool_t MVideo::ReadControl(MVideoCtrl &vctrl) const
{
    struct v4l2_control ctrl = { vctrl.fId, 0 };
    if (Ioctl(VIDIOC_G_CTRL, &ctrl)==-1)
        return kFALSE;

    vctrl.fValue = ctrl.value;

    return kTRUE;
}

// -------------------------------------------------------------
//
// Write the given value into the given control of the device.
// On success the value is stored in the given MVideoCtrl.
//
Bool_t MVideo::WriteControl(MVideoCtrl &vctrl, Int_t val) const
{
    if (val<vctrl.fMinimum)
    {
        gLog << err << "ERROR - Value of " << val << " below minimum of " << vctrl.fMinimum << " for " << vctrl.fName << endl;
        return kFALSE;
    }

    if (val>vctrl.fMaximum)
    {
        gLog << err << "ERROR - Value of " << val << " above maximum of " << vctrl.fMaximum << " for " << vctrl.fName << endl;
        return kFALSE;
    }

    struct v4l2_control ctrl = { vctrl.fId, val };
    if (Ioctl(VIDIOC_S_CTRL, &ctrl)==-1)
        return kFALSE;

    vctrl.fValue = val;

    return kTRUE;
}

// -------------------------------------------------------------
//
// Set all controls from a TEnv. Note that all whitespaces
// and colons in the control names (as defined by the name of
// the MVideoCtrls stored in fControls) are replaced by
// underscores.
//
Bool_t MVideo::SetControls(TEnv &env) const
{
    Bool_t rc = kTRUE;

    TIter Next(&fControls);
    TObject *o = 0;
    while ((o=Next()))
    {
        if (!env.Defined(o->GetName()))
            continue;

        TString str = env.GetValue(o->GetName(), "");
        str = str.Strip(TString::kBoth);
        str.ReplaceAll(" ", "_");
        str.ReplaceAll(":", "_");
        if (str.IsNull())
            continue;

        MVideoCtrl &ctrl = *static_cast<MVideoCtrl*>(o);

        const Int_t val = str=="default" || str=="def" ?
            ctrl.fDefault : env.GetValue(o->GetName(), 0);

        if (!WriteControl(ctrl, val))
            rc = kFALSE;
    }

    return rc;
}

template<class S>
Bool_t MVideo::Enumerate(vector<S> &vec, int request)
{
    for (int i=0; ; i++)
    {
        S input;
        input.index = i;

        const int rc = Ioctl(request, &input);
        if (rc<0)
            return kFALSE;
        if (rc==1)
            return kTRUE;

        vec.push_back(input);
    }

    return kFALSE;
}

void MVideo::PrintInputs() const
{
    gLog << all;
    for (vector<v4l2_input>::const_iterator it=fInputs.begin(); it!=fInputs.end(); it++)
    {
        gLog << "Input #" << it->index << endl;
        gLog << " - " << it->name << endl;
        gLog << " - " << (it->type==V4L2_INPUT_TYPE_CAMERA?"Camera":"Tuner") << endl;
        gLog << " - TV Standard: " << hex << it->std << dec << endl;

        gLog << " - Status: 0x" << hex << it->status;
        if (it->status&V4L2_IN_ST_NO_POWER)
            gLog << " NoPower";
        if (it->status&V4L2_IN_ST_NO_SIGNAL)
            gLog << " NoSignal";
        if (it->status&V4L2_IN_ST_NO_COLOR)
            gLog << " NoColor";
        if (it->status&V4L2_IN_ST_NO_H_LOCK)
            gLog << " NoHLock";
        if (it->status&V4L2_IN_ST_COLOR_KILL)
            gLog << " ColorKill";
        gLog << endl;

        /*
         TV Standard
         ===========
         #define V4L2_STD_PAL_B          ((v4l2_std_id)0x00000001)
         #define V4L2_STD_PAL_B1         ((v4l2_std_id)0x00000002)
         #define V4L2_STD_PAL_G          ((v4l2_std_id)0x00000004)
         #define V4L2_STD_PAL_H          ((v4l2_std_id)0x00000008)
         #define V4L2_STD_PAL_I          ((v4l2_std_id)0x00000010)
         #define V4L2_STD_PAL_D          ((v4l2_std_id)0x00000020)
         #define V4L2_STD_PAL_D1         ((v4l2_std_id)0x00000040)
         #define V4L2_STD_PAL_K          ((v4l2_std_id)0x00000080)

         #define V4L2_STD_PAL_M          ((v4l2_std_id)0x00000100)
         #define V4L2_STD_PAL_N          ((v4l2_std_id)0x00000200)
         #define V4L2_STD_PAL_Nc         ((v4l2_std_id)0x00000400)
         #define V4L2_STD_PAL_60         ((v4l2_std_id)0x00000800)
         V4L2_STD_PAL_60 is a hybrid standard with 525 lines, 60 Hz refresh rate, and PAL color modulation with a 4.43 MHz color subcarrier. Some PAL video recorders can play back NTSC tapes in this mode for display on a 50/60 Hz agnostic PAL TV.

         #define V4L2_STD_NTSC_M         ((v4l2_std_id)0x00001000)
         #define V4L2_STD_NTSC_M_JP      ((v4l2_std_id)0x00002000)
         #define V4L2_STD_NTSC_443       ((v4l2_std_id)0x00004000)
         V4L2_STD_NTSC_443 is a hybrid standard with 525 lines, 60 Hz refresh rate, and NTSC color modulation with a 4.43 MHz color subcarrier.

         #define V4L2_STD_NTSC_M_KR      ((v4l2_std_id)0x00008000)

         #define V4L2_STD_SECAM_B        ((v4l2_std_id)0x00010000)
         #define V4L2_STD_SECAM_D        ((v4l2_std_id)0x00020000)
         #define V4L2_STD_SECAM_G        ((v4l2_std_id)0x00040000)
         #define V4L2_STD_SECAM_H        ((v4l2_std_id)0x00080000)
         #define V4L2_STD_SECAM_K        ((v4l2_std_id)0x00100000)
         #define V4L2_STD_SECAM_K1       ((v4l2_std_id)0x00200000)
         #define V4L2_STD_SECAM_L        ((v4l2_std_id)0x00400000)
         #define V4L2_STD_SECAM_LC       ((v4l2_std_id)0x00800000)


         // ATSC/HDTV
         #define V4L2_STD_ATSC_8_VSB     ((v4l2_std_id)0x01000000)
         #define V4L2_STD_ATSC_16_VSB    ((v4l2_std_id)0x02000000)
         V4L2_STD_ATSC_8_VSB and V4L2_STD_ATSC_16_VSB are U.S. terrestrial digital TV standards. Presently the V4L2 API does not support digital TV. See also the Linux DVB API at http://linuxtv.org.

         #define V4L2_STD_PAL_BG   (V4L2_STD_PAL_B   | V4L2_STD_PAL_B1    | V4L2_STD_PAL_G)
         #define V4L2_STD_B        (V4L2_STD_PAL_B   | V4L2_STD_PAL_B1    | V4L2_STD_SECAM_B)
         #define V4L2_STD_GH       (V4L2_STD_PAL_G   | V4L2_STD_PAL_H     | V4L2_STD_SECAM_G  | V4L2_STD_SECAM_H)
         #define V4L2_STD_PAL_DK   (V4L2_STD_PAL_D   | V4L2_STD_PAL_D1    | V4L2_STD_PAL_K)
         #define V4L2_STD_PAL      (V4L2_STD_PAL_BG  | V4L2_STD_PAL_DK    | V4L2_STD_PAL_H    | V4L2_STD_PAL_I)
         #define V4L2_STD_NTSC     (V4L2_STD_NTSC_M  | V4L2_STD_NTSC_M_JP | V4L2_STD_NTSC_M_KR)
         #define V4L2_STD_MN       (V4L2_STD_PAL_M   | V4L2_STD_PAL_N     | V4L2_STD_PAL_Nc   | V4L2_STD_NTSC)
         #define V4L2_STD_SECAM_DK (V4L2_STD_SECAM_D | V4L2_STD_SECAM_K   | V4L2_STD_SECAM_K1)
         #define V4L2_STD_DK       (V4L2_STD_PAL_DK  | V4L2_STD_SECAM_DK)
         #define V4L2_STD_SECAM    (V4L2_STD_SECAM_B | V4L2_STD_SECAM_G   | V4L2_STD_SECAM_H  | V4L2_STD_SECAM_DK | V4L2_STD_SECAM_L | V4L2_STD_SECAM_LC)
         #define V4L2_STD_525_60   (V4L2_STD_PAL_M   | V4L2_STD_PAL_60    | V4L2_STD_NTSC     | V4L2_STD_NTSC_443)
         #define V4L2_STD_625_50   (V4L2_STD_PAL     | V4L2_STD_PAL_N     | V4L2_STD_PAL_Nc   | V4L2_STD_SECAM)
         #define V4L2_STD_UNKNOWN  0
         #define V4L2_STD_ALL      (V4L2_STD_525_60  | V4L2_STD_625_50)
         */

         /*
          Status:
         =======
         General
         V4L2_IN_ST_NO_POWER	0x00000001	Attached device is off.
         V4L2_IN_ST_NO_SIGNAL	0x00000002
         V4L2_IN_ST_NO_COLOR	0x00000004	The hardware supports color decoding, but does not detect color modulation in the signal.

         Analog Video
         V4L2_IN_ST_NO_H_LOCK	0x00000100	No horizontal sync lock.
         V4L2_IN_ST_COLOR_KILL	0x00000200	A color killer circuit automatically disables color decoding when it detects no color modulation. When this flag is set the color killer is enabled and has shut off color decoding.

         Digital Video
         V4L2_IN_ST_NO_SYNC	0x00010000	No synchronization lock.
         V4L2_IN_ST_NO_EQU	0x00020000	No equalizer lock.
         V4L2_IN_ST_NO_CARRIER	0x00040000	Carrier recovery failed.

         VCR and Set-Top Box
         V4L2_IN_ST_MACROVISION	0x01000000	Macrovision is an analog copy prevention system mangling the video signal to confuse video recorders. When this flag is set Macrovision has been detected.
         V4L2_IN_ST_NO_ACCESS	0x02000000	Conditional access denied.
         V4L2_IN_ST_VTR	        0x04000000	VTR time constant. [?]
         */
    }
}

void MVideo::PrintStandards() const
{
    gLog << all;
    for (vector<v4l2_standard>::const_iterator it=fStandards.begin(); it!=fStandards.end(); it++)
    {
        gLog << "Index #" << it->index << endl;
        gLog << " - TV Standard: " << it->name << hex << "(" << it->id << ")" << dec << endl;
        gLog << " - FPS: " << it->frameperiod.numerator << "/" << it->frameperiod.denominator << endl;
        gLog << " - Lines: " << it->framelines << endl;
    }
}

// -------------------------------------------------------------
//
// Open channel ch of the device
//
Bool_t MVideo::Open(Int_t ch)
{
    const Bool_t rc = Init(ch);
    if (!rc)
        Close();
    return rc;
}

// -------------------------------------------------------------
//
// Open a channel of the device and retriev all necessary
// informations from the driver. Initialize the shared
// memory. Other access methods are not supported yet.
//
Bool_t MVideo::Init(Int_t channel)
{
    if (IsOpen())
    {
        gLog << warn << "WARNING - Device " << fPath << " already open." << endl;
        return kTRUE;
    }

    gLog << all << "Opening " << fPath << "... " << flush;
    do
    {
        fFileDesc = open(fPath, O_RDWR|O_NONBLOCK, 0);
        usleep(1);
    }
    while (errno==19 && fFileDesc==-1);

    if (fFileDesc == -1)
    {
        gLog << err << "ERROR: " << strerror(errno) << endl;
        return kFALSE;
    }

    gLog << "done (" << fFileDesc << ")." << endl;

    // Close device on exit
    if (fcntl(fFileDesc, F_SETFD, FD_CLOEXEC)<0)
    {
        gLog << err << "ERROR - Call to fnctl (F_SETFD, FD_CLOEXEC) failed." << endl;
        return kFALSE;
    }



/*
    if (!Enumerate(fInputs, VIDIOC_ENUMINPUT))
    {
        gLog << err << "ERROR - Could not enumerate inputs." << endl;
        return kFALSE;
    }
    PrintInputs();

    if (!Enumerate(fStandards, VIDIOC_ENUMSTD))
    {
        gLog << err << "ERROR - Could not enumerate inputs." << endl;
        return kFALSE;
    }
    PrintStandards();
   */

    int index = 3;
    if (Ioctl(VIDIOC_S_INPUT, &index)==-1)
    {
        gLog << err << "ERROR - Could not set input." << endl;
        return kFALSE;
    }

    //check the input
    if (Ioctl(VIDIOC_G_INPUT, &index))
    {
        gLog << err << "ERROR - Could not get input." << endl;
        return kFALSE;
    }

    v4l2_input input;
    memset(&input, 0, sizeof (input));
    input.index = index;
    if (Ioctl(VIDIOC_ENUMINPUT, &input))
    {
        gLog << err << "ERROR - Could enum input." << endl;
        return kFALSE;
    }
    gLog << "*** Input: " << input.name << " (" << input.index << ")" << endl;

    v4l2_std_id st = 4;//standard.id;
    if (Ioctl (VIDIOC_S_STD, &st))
    {
        gLog << err << "ERROR - Could not set standard." << endl;
        return kFALSE;
    }

    v4l2_capability cap;
    if (Ioctl(VIDIOC_QUERYCAP, &cap))
    {
        gLog << err << "ERROR - Could not get capabilities." << endl;
        return kFALSE;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        gLog << err << "ERROR - No capture capabaility." << endl;
        return kFALSE;
    }

    v4l2_cropcap cropcap;
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (Ioctl(VIDIOC_CROPCAP, &cropcap)==-1)
    {
    }

    v4l2_crop crop;
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if (Ioctl(VIDIOC_S_CROP, &crop))
    {
        gLog << err << "Could not reset cropping." << endl;
        return kFALSE;
    }

    v4l2_format fmt;
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = 768;
    fmt.fmt.pix.height      = 576;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;

    if (Ioctl(VIDIOC_S_FMT, &fmt)==-1)
    {
        gLog << err << "ERROR - Could not set format." << endl;
        return kFALSE;
    }
    // The image format must be selected before buffers are allocated,
    // with the VIDIOC_S_FMT ioctl. When no format is selected the driver
    // may use the last, possibly by another application requested format.

    v4l2_requestbuffers reqbuf;
    memset (&reqbuf, 0, sizeof (reqbuf));

    reqbuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count  = 4;//125;

    if (Ioctl(VIDIOC_REQBUFS, &reqbuf)==-1)
    {
        gLog << err << "ERROR - Couldn't setup frame buffers." << endl;
        return kFALSE;
    }

    gLog << all << "Allocated " << reqbuf.count << " frame buffers." << endl;

    for (unsigned int i=0; i<reqbuf.count; i++)
    {
        v4l2_buffer buffer;
        memset (&buffer, 0, sizeof (buffer));

        buffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index  = i;

        if (Ioctl(VIDIOC_QUERYBUF, &buffer))
        {
            gLog << err << "ERROR - Request of frame buffer " << i << " failed." << endl;
            return kFALSE;
        }

        void *ptr = mmap(NULL, buffer.length,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         fFileDesc, buffer.m.offset);

        if (MAP_FAILED == ptr)
        {

            gLog << err << "ERROR - Could not allocate shared memory." << endl;
            return kFALSE;
                // If you do not exit here you should unmap() and free()
                // the buffers mapped so far.
                //perror ("mmap");
                //exit (EXIT_FAILURE);
        }

        fBuffers.push_back(make_pair(buffer, ptr));
    }

    return kTRUE;
}

Bool_t MVideo::Start()
{
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (Ioctl(VIDIOC_STREAMON, &type)==-1)
    {
        gLog << err << "ERROR - Couldn't start capturing." << endl;
        return kFALSE;
    }

    cout << "*** Stream on" << endl;

    return kTRUE;
}

// -------------------------------------------------------------
//
// Close device. Free the shared memory
//
Int_t MVideo::Close()
{
    //    if (!IsOpen())
    //        return kTRUE;
/*
    if (Ioctl(VIDIOC_STREAMON, &fBuffers[0])==-1)
    {
        gLog << err << "ERROR - Couldn't start capturing." << endl;
        return kFALSE;
    }
*/
    Bool_t rc = kTRUE;

    gLog << all << "Closing " << fPath << " (" << fFileDesc << ")... " << flush;
    if (fFileDesc != -1)
    {
        if (close(fFileDesc)<0)
        {
            gLog << err << "ERROR!" << endl;
            rc = kFALSE;
        }
        fFileDesc = -1;
    }
    gLog << "done." << endl;

    // unmap device memory
    for (vector<pair<v4l2_buffer,void*> >::iterator it=fBuffers.begin(); it!=fBuffers.end(); it++)
    {
        munmap(it->second, it->first.length);
        fBuffers.erase(it);
    }

    Reset();

    return rc;
}

// -------------------------------------------------------------
//
// Instruct hardware to start capture into framebuffer frame
//
Bool_t MVideo::CaptureStart(unsigned int frame) const
{
    frame %= fBuffers.size();

//    cout << "*** CaptureStart " << frame << endl;

    if (Ioctl(VIDIOC_QBUF, const_cast<v4l2_buffer*>(&fBuffers[frame].first))==-1)
    {
        gLog << err << "ERROR - Couldn't buffer " << frame << "." << endl;
        return kFALSE;
    }

//    cout << "*** " << errno << endl;

    return kTRUE;

    /*
    struct video_mmap gb =
    {
        frame,                           // frame
        fCaps.maxheight, fCaps.maxwidth, // height, width
        VIDEO_PALETTE_RGB24             // palette
    };

#ifdef DEBUG
    gLog << dbg << "CapturStart(" << frame << ")" << endl;
#endif

    //
    // capture frame
    //
    if (Ioctl(VIDIOCMCAPTURE, &gb) != -1)
        return kTRUE;

//    if (errno == EAGAIN)
    gLog << err;
    gLog << "ERROR - Couldn't start capturing frame " << frame << "." << endl;
    gLog << "        Maybe your card doesn't support VIDEO_PALETTE_RGB24." << endl;
    return kFALSE;
    */
}

// -------------------------------------------------------------
//
// Wait until hardware has finished capture into framebuffer frame
//
Int_t MVideo::CaptureWait(unsigned int frame, unsigned char **ptr) const
{
    frame %= fBuffers.size();

    if (ptr)
        *ptr = NULL;

//    const int SYNC_TIMEOUT = 1;

//#ifdef DEBUG
//    cout << "*** CaptureWait " << frame << endl;
//#endif

    //alarm(SYNC_TIMEOUT);
    const Int_t rc = Ioctl(VIDIOC_DQBUF, const_cast<v4l2_buffer*>(&fBuffers[frame].first), false);
    if (rc==-4)
    {
        //cout << "ERROR - Waiting for frame " << frame << " timed out." << endl;
        return kSKIP;
    }
    //alarm(0);

    if (rc==-1)
    {
        gLog << err << "ERROR - Waiting for " << frame << " frame failed." << endl;
        return kFALSE;
    }

    if (ptr)
        *ptr = static_cast<unsigned char*>(fBuffers[frame].second);

    return kTRUE;
}

// -------------------------------------------------------------
//
// Change the channel of a priviously opened device
//
Int_t MVideo::SetChannel(Int_t chan)
{
    return kSKIP;

    if (fChannel.channel==chan)
        return kSKIP;

    if (chan<0 || chan>=fCaps.channels)
    {
        gLog << err << "ERROR - Set channel " << chan << " out of range." << endl;
        return kFALSE;
    }

    // Switch to channel
    struct video_channel ch = { chan, "", 0, 0, 0, 0 };
    if (Ioctl(VIDIOCSCHAN, &ch)==-1)
    {
        gLog << err << "ERROR - Couldn't switch to channel " << chan << "." << endl;
        gLog << "        You might need a bttv version > 0.5.13" << endl;
        return kFALSE;
    }

    // Get information about channel
    if (Ioctl(VIDIOCGCHAN, &ch)==-1)
    {
        gLog << err << "ERROR - Getting information for channel " << chan << " failed." << endl;
        return kFALSE;
    }

    memcpy(&fChannel, &ch, sizeof(fChannel));

    gLog << all << "Switched to channel " << chan << endl;

    return kTRUE;
}

// -------------------------------------------------------------
//
// Has the device capture capabilities?
//
Bool_t MVideo::CanCapture() const
{
    return fCaps.type&VID_TYPE_CAPTURE;
}

// -------------------------------------------------------------
//
// Has a tuner
//
Bool_t MVideo::HasTuner() const
{
    return fCaps.type&VID_TYPE_TUNER;
}

// -------------------------------------------------------------
//
// Returns the number of frame buffers which can be used
//
Int_t MVideo::GetNumBuffers() const
{
    return fBuffers.size();
}

// -------------------------------------------------------------
//
// Maximum width of the frame which can be captured
//
Int_t MVideo::GetWidth() const
{
    return 768;//fCaps.maxwidth;
}

// -------------------------------------------------------------
//
// Maximum height of the frame which can be captured
//
Int_t MVideo::GetHeight() const
{
    return 576;//fCaps.maxheight;
}

// -------------------------------------------------------------
//
// Return the device type as string
//
TString MVideo::GetDevType(int type) const
{
    TString rc;
    if (CanCapture())
        rc += " capture";
    if (HasTuner())
        rc += " tuner";
    if (type&VID_TYPE_TELETEXT)
        rc += " teletext";
    if (type&VID_TYPE_OVERLAY)
        rc += " overlay";
    if (type&VID_TYPE_CHROMAKEY)
        rc += " chromakey";
    if (type&VID_TYPE_CLIPPING)
        rc += " clipping";
    if (type&VID_TYPE_FRAMERAM)
        rc += " frameram";
    if (type&VID_TYPE_SCALES)
        rc += " scales";
    if (type&VID_TYPE_MONOCHROME)
        rc += " monochrom";
    if (type&VID_TYPE_SUBCAPTURE)
        rc += " subcapature";
    return rc;
}

TString MVideo::GetTunerFlags(Int_t flags) const
{
    TString rc;
    if (flags&VIDEO_TUNER_PAL)
        rc += " PAL";
    if (flags&VIDEO_TUNER_NTSC)
        rc += " NTSC";
    if (flags&VIDEO_TUNER_SECAM)
        rc += " SECAM";
    if (flags&VIDEO_TUNER_LOW)
        rc += " kHz";
    if (flags&VIDEO_TUNER_NORM)
        rc += " CanSetNorm";
    if (flags&VIDEO_TUNER_STEREO_ON)
        rc += " StereoOn";
    return rc;
}

TString MVideo::GetTunerMode(Int_t mode) const
{
    switch (mode)
    {
    case VIDEO_MODE_PAL:
        return "PAL";
    case VIDEO_MODE_NTSC:
        return "NTSC";
    case VIDEO_MODE_SECAM:
        return "SECAM";
    case VIDEO_MODE_AUTO:
        return "AUTO";
    }
    return "undefined";
}

// -------------------------------------------------------------
//
// Return the channel flags as string
//
TString MVideo::GetChannelFlags(Int_t flags) const
{
    TString rc = "video";
    if (flags&VIDEO_VC_TUNER)
        rc += " tuner";
    if (flags&VIDEO_VC_AUDIO)
        rc += " audio";
//    if (flags&VIDEO_VC_NORM)
//        rc += " normsetting";
    return rc;
}

// -------------------------------------------------------------
//
// Return the channel type as string
//
TString MVideo::GetChannelType(Int_t type) const
{
    if (type&VIDEO_TYPE_TV)
        return "TV";
    if (type&VIDEO_TYPE_CAMERA)
        return "Camera";
    return "unknown";
}

// -------------------------------------------------------------
//
// Return the palette pal as string
//
TString MVideo::GetPalette(Int_t pal) const
{
    switch (pal)
    {
    case VIDEO_PALETTE_GREY:
        return "VIDEO_PALETTE_GREY: Linear intensity grey scale";
    case VIDEO_PALETTE_HI240:
        return "VIDEO_PALETTE_HI240: BT848 8-bit color cube";
    case VIDEO_PALETTE_RGB565:
        return "VIDEO_PALETTE_RGB565: RGB565 packed into 16-bit words";
    case VIDEO_PALETTE_RGB555:
        return "VIDEO_PALETTE_RGB555: RGB555 packed into 16-bit words, top bit undefined";
    case VIDEO_PALETTE_RGB24:
        return "VIDEO_PALETTE_RGB24: RGB888 packed into 24-bit words";
    case VIDEO_PALETTE_RGB32:
        return "VIDEO_PALETTE_RGB32: RGB888 packed into the low three bytes of 32-bit words. Top bits undefined.";
    case VIDEO_PALETTE_YUV422:
        return "VIDEO_PALETTE_YUV422: Video style YUV422 - 8-bit packed, 4-bit Y, 2-bits U, 2-bits V";
    case VIDEO_PALETTE_YUYV:
        return "VIDEO_PALETTE_YUYV: YUYV";
    case VIDEO_PALETTE_UYVY:
        return "VIDEO_PALETTE_UYVY: UYVY";
    case VIDEO_PALETTE_YUV420:
        return "VIDEO_PALETTE_YUV420: YUV420";
    case VIDEO_PALETTE_YUV411:
        return "VIDEO_PALETTE_YUV411: YUV411";
    case VIDEO_PALETTE_RAW:
        return "VIDEO_PALETTE_RAW: Raw capture (Bt848)";
    case VIDEO_PALETTE_YUV422P:
        return "VIDEO_PALETTE_YUV422P: YUV 4:2:2 planar";
    case VIDEO_PALETTE_YUV411P:
        return "VIDEO_PALETTE_YUV411P: YUV 4:1:1 planar";
    }
    return "unknown";
}

// -------------------------------------------------------------
//
// Print informations about the device, the capabilities, the
// channel and all available information
//
void MVideo::Print() const
{
    gLog << all << dec;

    gLog << "Device " << fPath << " " << (fFileDesc>0?"open":"closed") << "." << endl;

    if (fFileDesc<=0)
        return;

    gLog  << " - Name:       " << fCaps.name << endl;
    gLog  << " - DevType:   "  << GetDevType(fCaps.type) << endl;
    gLog  << " - Channels:   " << fCaps.channels << endl;
    gLog  << " - Audios:     " << fCaps.audios << endl;
    gLog  << " - Size:       ";
    gLog  << fCaps.minwidth << "x" << fCaps.minheight << " to ";
    gLog  << fCaps.maxwidth << "x" << fCaps.maxheight << endl;
    gLog  << endl;
    if (fChannel.channel>=0)
    {
        gLog  << " - Channel:    " << fChannel.channel << " (" << fChannel.name << ")" << endl;
        gLog  << " - IsA:        " << GetChannelType(fChannel.type) << " with " << GetChannelFlags(fChannel.flags) << " (" << fChannel.flags << ")" << endl;
        //if (fChannel.flags&VIDEO_VC_NORM)
        gLog  << " - Norm:       " << fChannel.norm << endl;
        gLog  << endl;
    }

    if (fAbil.tuner>=0)
    {
        gLog << " - Tuner:           " << fAbil.tuner << endl;
        gLog << " - Name:            " << fAbil.name << endl;
 //       gLog << " - Tuner Range:     " << fAbil.rangelow << " - " << fAbil.rangehigh << endl;
        gLog << " - Tuner flags:    " << GetTunerFlags(fAbil.flags) << " (" << fAbil.flags << ")" << endl;
        gLog << " - Tuner mode:      " << GetTunerMode(fAbil.mode) << " (" << fAbil.mode << ")" <<endl;
        gLog << " - Signal Strength: " << fAbil.signal << endl;
    }

    gLog  << " - Brightness: " << fProp.brightness << endl;
    gLog  << " - Hue:        " << fProp.hue << endl;
    gLog  << " - Color:      " << fProp.colour << endl;
    gLog  << " - Contrast:   " << fProp.contrast << endl;
    gLog  << " - Whiteness:  " << fProp.whiteness << endl;
    gLog  << " - Depth:      " << fProp.depth << endl;
    gLog  << " - Palette:    " << GetPalette(fProp.palette) << " (" << fProp.palette << ")" << endl;
    gLog  << endl;

//    gLog  << " - BufferSize: 0x" << hex << fBuffer.size << " (" << dec << fBuffer.frames << " frames)" << endl;
//    gLog  << " - Offsets:   " << hex;
//    for (int i=0; i<fBuffer.frames; i++)
//        gLog  << " 0x" << fBuffer.offsets[i];
//    gLog  << dec << endl;

    gLog << inf2 << "Controls:" << endl;
    fControls.Print();
}

/*
void MVideo::SetPicPar(int bright, int hue, int contrast)
{
    struct video_picture pict;

    Ioctl(VIDIOCGPICT, &pict);  // get

    if (contrast != -1)
        pict.contrast = contrast;

    if (bright != -1)
        pict.brightness = bright;

    if (hue != -1)
	pict.hue = hue;

    Ioctl(VIDIOCSPICT, &pict);  //set
}

void MVideo::GetPicPar(int *bright, int *hue, int *contrast)
{
    struct video_picture pict;

    Ioctl(VIDIOCGPICT, &pict);   // get

    *contrast = pict.contrast;
    *bright   = pict.brightness;
    *hue      = pict.hue;
}
*/
