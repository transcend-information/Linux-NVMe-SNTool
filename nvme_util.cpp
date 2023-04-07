#include "nvme_util.h"
#include <fcntl.h>

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