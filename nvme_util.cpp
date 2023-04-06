#include "nvme_util.h"
#include <fcntl.h>
#include "linux_nvme_ioctl.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#define O_RDONLY	     00
#ifndef O_NONBLOCK
# define O_NONBLOCK	  04000
#endif



nvme_Device::nvme_Device()
{

}
nvme_Device::nvme_Device(const char * dev_name, const char * req_type, unsigned nsid)
{
    nvmeopts.drive_info = true;
    nvmeopts.smart_check_status = true;

    m_nsid = nsid;
    m_info.dev_name = dev_name;
    m_info.info_name = dev_name;
    m_info.dev_type = "nvme";
    m_info.req_type = req_type;
    m_fd = -1;
    m_flags = O_RDONLY | O_NONBLOCK;
    m_retry_flags = -1;
}

int nvme_Device::nvmeOpen()
{
    m_fd = ::open(m_info.dev_name.c_str(), m_flags);

    if (m_fd < 0 && errno == EROFS && m_retry_flags != -1)
        // Retry
        m_fd = ::open(m_info.dev_name.c_str(), m_retry_flags);

    if (m_fd < 0) {
        //QMessageBox::information(NULL, "Error", "open fail", NULL);
        //    if (errno == EBUSY && (m_flags & O_EXCL))
        //      // device is locked
        //      return set_err(EBUSY,
        //        "The requested controller is used exclusively by another process!\n"
        //        "(e.g. smartctl or smartd)\n"
        //        "Please quit the impeding process or try again later...");
        //    return set_err((errno==ENOENT || errno==ENOTDIR) ? ENODEV : errno);
        return m_fd;
    }

    if (m_fd >= 0) {
        // sets FD_CLOEXEC on the opened device file descriptor.  The
        // descriptor is otherwise leaked to other applications (mail
        // sender) which may be considered a security risk and may result
        // in AVC messages on SELinux-enabled systems.
        if (-1 == fcntl(m_fd, F_SETFD, FD_CLOEXEC))
        {
            // TODO: Provide an error printing routine in class smart_interface
            //    QMessageBox::information(NULL, "Error", "fcntl(set  FD_CLOEXEC) failed", NULL);
            //qDebug()<<"fcntl(set  FD_CLOEXEC) failed";
        //pout("fcntl(set  FD_CLOEXEC) failed, errno=%d [%s]\n", errno, strerror(errno));
        }
    }

    if (!m_nsid) {
        // Use actual NSID (/dev/nvmeXnN) if available,
        // else use broadcast namespace (/dev/nvmeX)
        int nsid = ioctl(m_fd, NVME_IOCTL_ID, (void*)0);
        m_nsid = nsid;
    }

    return m_fd;
}

void nvme_Device::swap2(char *location){
    char tmp=*location;
    *location=*(location+1);
    *(location+1)=tmp;
    return;
}

// swap four bytes.  Point to low address
void nvme_Device::swap4(char *location){
    char tmp=*location;
    *location=*(location+3);
    *(location+3)=tmp;
    swap2(location+1);
    return;
}

// swap eight bytes.  Points to low address
void nvme_Device::swap8(char *location){
    char tmp=*location;
    *location=*(location+7);
    *(location+7)=tmp;
    tmp=*(location+1);
    *(location+1)=*(location+6);
    *(location+6)=tmp;
    swap4(location+2);
    return;
}

bool nvme_Device::nvme_read_id_ctrl(nvme_id_ctrl & id_ctrl)
{
    if (!nvme_read_identify(0, 0x01, &id_ctrl, sizeof(id_ctrl)))
        return false;

    if (isbigendian()) {
        swapx(&id_ctrl.vid);
        swapx(&id_ctrl.ssvid);
        swapx(&id_ctrl.cntlid);
        swapx(&id_ctrl.oacs);
        swapx(&id_ctrl.wctemp);
        swapx(&id_ctrl.cctemp);
        swapx(&id_ctrl.mtfa);
        swapx(&id_ctrl.hmpre);
        swapx(&id_ctrl.hmmin);
        swapx(&id_ctrl.rpmbs);
        swapx(&id_ctrl.nn);
        swapx(&id_ctrl.oncs);
        swapx(&id_ctrl.fuses);
        swapx(&id_ctrl.awun);
        swapx(&id_ctrl.awupf);
        swapx(&id_ctrl.acwu);
        swapx(&id_ctrl.sgls);
        for (int i = 0; i < 32; i++) {
            swapx(&id_ctrl.psd[i].max_power);
            swapx(&id_ctrl.psd[i].entry_lat);
            swapx(&id_ctrl.psd[i].exit_lat);
            swapx(&id_ctrl.psd[i].idle_power);
            swapx(&id_ctrl.psd[i].active_power);
        }
    }

    return true;
}

bool nvme_Device::nvme_read_id_ns(unsigned nsid, nvme_id_ns & id_ns)
{
    if (!nvme_read_identify(nsid, 0x00, &id_ns, sizeof(id_ns)))
        return false;

    if (isbigendian()) {
        swapx(&id_ns.nsze);
        swapx(&id_ns.ncap);
        swapx(&id_ns.nuse);
        swapx(&id_ns.nawun);
        swapx(&id_ns.nawupf);
        swapx(&id_ns.nacwu);
        swapx(&id_ns.nabsn);
        swapx(&id_ns.nabo);
        swapx(&id_ns.nabspf);
        for (int i = 0; i < 16; i++)
            swapx(&id_ns.lbaf[i].ms);
    }

    return true;
}

bool nvme_Device::nvme_pass_through(const nvme_cmd_in & in)
{
    nvme_cmd_out out;
    nvme_passthru_cmd pt;
    memset(&pt, 0, sizeof(pt));

    pt.opcode = in.opcode;
    pt.nsid = in.nsid;
    pt.addr = (uint64_t)in.buffer;
    pt.data_len = in.size;
    pt.cdw10 = in.cdw10;
    pt.cdw11 = in.cdw11;
    pt.cdw12 = in.cdw12;
    pt.cdw13 = in.cdw13;
    pt.cdw14 = in.cdw14;
    pt.cdw15 = in.cdw15;
    // Kernel default for NVMe admin commands is 60 seconds
    // pt.timeout_ms = 60 * 1000;
    
    int status = ioctl(m_fd, NVME_IOCTL_ADMIN_CMD, &pt);
	if (status < 0)
        //return set_err(errno, "NVME_IOCTL_ADMIN_CMD: %s", strerror(errno));
        return false;

    if (status > 0)
        //return set_nvme_err(out, status);
        return false;

    out.result = pt.result;
    bool ok = true;

    //  if (   dont_print_serial_number && ok
    //      && in.opcode == nvme_admin_identify && in.cdw10 == 0x01) {
    //        // Invalidate serial number
    //        nvme_id_ctrl & id_ctrl = *reinterpret_cast<nvme_id_ctrl *>(in.buffer);
    //        memset(id_ctrl.sn, 'X', sizeof(id_ctrl.sn));
    //  }

    //  if (nvme_debugmode) {
    //    if (start_usec >= 0) {
    //      int64_t duration_usec = smi()->get_timer_usec() - start_usec;
    //      if (duration_usec >= 500)
    //        pout("  [Duration: %.3fs]\n", duration_usec / 1000000.0);
    //    }

    //    if (!ok) {
    //      pout(" [NVMe call failed: ");
    //      if (out.status_valid)
    //        pout("NVMe Status=0x%04x", out.status);
    //      else
    //        pout("%s", device->get_errmsg());
    //    }
    //    else {
    //      pout(" [NVMe call succeeded: result=0x%08x", out.result);
    //      if (nvme_debugmode > 1 && in.direction() == nvme_cmd_in::data_in) {
    //        pout("\n");
    //        debug_hex_dump(in.buffer, in.size);
    //        pout(" ");
    //      }
    //    }
    //    pout("]\n");
    //  }

    return ok;
}

bool nvme_Device::nvme_read_identify(unsigned nsid,
                                     unsigned char cns, void * data, unsigned size)
{
    memset(data, 0, size);
    nvme_cmd_in in;
    in.set_data_in(nvme_admin_identify, data, size);
    in.nsid = nsid;
    in.cdw10 = cns;

    return nvme_pass_through(in);
     
}