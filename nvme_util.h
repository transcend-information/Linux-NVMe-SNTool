#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <linux/types.h>

#ifndef NVME_UTIL_H
#define NVME_UTIL_H

class nvme_Device
{
public:
    nvme_Device();
    nvme_Device(const char * dev_name, const char * req_type, unsigned nsid);

    struct device_info {
        device_info()
        { }
        device_info(const char * d_name, const char * d_type, const char * r_type)
            : dev_name(d_name), info_name(d_name),
              dev_type(d_type), req_type(r_type)
        { }

        std::string dev_name;  ///< Device (path)name
        std::string info_name; ///< Informal name
        std::string dev_type;  ///< Actual device type
        std::string req_type;  ///< Device type requested by user, empty if none
    };
    device_info m_info;

    struct nvme_cmd_in
    {
        unsigned char opcode; ///< Opcode (CDW0 07:00)
        unsigned nsid; ///< Namespace ID
        unsigned cdw10, cdw11, cdw12, cdw13, cdw14, cdw15; ///< Cmd specific

        void * buffer; ///< Pointer to data buffer
        unsigned size; ///< Size of buffer

        enum {
            no_data = 0x0, data_out = 0x1, data_in = 0x2, data_io = 0x3
        };

        /// Get I/O direction from opcode
        unsigned char direction() const
        { return (opcode & 0x3); }

        // Prepare for DATA IN command
        void set_data_in(unsigned char op, void * buf, unsigned sz)
        {
            opcode = op;
            if (direction() != data_in)
            {
                //   QMessageBox::information(NULL, "Error", "invalid opcode", NULL);

            }
            buffer = buf;
            size = sz;
        }

        nvme_cmd_in()
            : opcode(0), nsid(0),
              cdw10(0), cdw11(0), cdw12(0), cdw13(0), cdw14(0), cdw15(0),
              buffer(0), size(0)
        { }
    };

    /// NVMe pass through output parameters
    struct nvme_cmd_out
    {
        unsigned result; ///< Command specific result (DW0)
        unsigned short status; ///< Status Field (DW3 31:17)
        bool status_valid; ///< true if status is valid

        nvme_cmd_out()
            : result(0), status(0), status_valid(false)
        { }
    };

    bool nvme_pass_through(const nvme_cmd_in & in);

    unsigned m_nsid;

    struct nvme_id_power_state {
        unsigned short  max_power; // centiwatts
        unsigned char   rsvd2;
        unsigned char   flags;
        unsigned int    entry_lat; // microseconds
        unsigned int    exit_lat;  // microseconds
        unsigned char   read_tput;
        unsigned char   read_lat;
        unsigned char   write_tput;
        unsigned char   write_lat;
        unsigned short  idle_power;
        unsigned char   idle_scale;
        unsigned char   rsvd19;
        unsigned short  active_power;
        unsigned char   active_work_scale;
        unsigned char   rsvd23[9];
    };

    struct nvme_id_ctrl {
        unsigned short  vid;
        unsigned short  ssvid;
        char            sn[20];
        char            mn[40];
        char            fr[8];
        unsigned char   rab;
        unsigned char   ieee[3];
        unsigned char   cmic;
        unsigned char   mdts;
        unsigned short  cntlid;
        unsigned int    ver;
        unsigned int    rtd3r;
        unsigned int    rtd3e;
        unsigned int    oaes;
        unsigned char   rsvd96[160];
        unsigned short  oacs;
        unsigned char   acl;
        unsigned char   aerl;
        unsigned char   frmw;
        unsigned char   lpa;
        unsigned char   elpe;
        unsigned char   npss;
        unsigned char   avscc;
        unsigned char   apsta;
        unsigned short  wctemp;
        unsigned short  cctemp;
        unsigned short  mtfa;
        unsigned int    hmpre;
        unsigned int    hmmin;
        unsigned char   tnvmcap[16];
        unsigned char   unvmcap[16];
        unsigned int    rpmbs;
        unsigned char   rsvd316[196];
        unsigned char   sqes;
        unsigned char   cqes;
        unsigned char   rsvd514[2];
        unsigned int    nn;
        unsigned short  oncs;
        unsigned short  fuses;
        unsigned char   fna;
        unsigned char   vwc;
        unsigned short  awun;
        unsigned short  awupf;
        unsigned char   nvscc;
        unsigned char   rsvd531;
        unsigned short  acwu;
        unsigned char   rsvd534[2];
        unsigned int    sgls;
        unsigned char   rsvd540[1508];
        struct nvme_id_power_state  psd[32];
        unsigned char   vs[1024];
    };

    
    int nvmeOpen();

    bool nvme_read_id_ctrl(nvme_id_ctrl & id_ctrl);
    bool nvme_read_identify(unsigned nsid,
                            unsigned char cns, void * data, unsigned size);

    void swap2(char *location);
    void swap4(char *location);
    void swap8(char *location);
    // Typesafe variants using overloading
    inline void swapx(unsigned short * p)
    { swap2((char*)p); }
    inline void swapx(unsigned int * p)
    { swap4((char*)p); }
    inline void swapx(uint64_t * p)
    { swap8((char*)p); }

private:
    int m_fd; ///< filedesc, -1 if not open.
    int m_flags; ///< Flags for ::open()
    int m_retry_flags; ///< Flags to retry ::open(), -1 if no retry
};

inline bool isbigendian()
{
#ifdef WORDS_BIGENDIAN
    return true;
#else
    return false;
#endif
}

#define nvme_admin_cmd nvme_passthru_cmd
#define NVME_IOCTL_ID		_IO('N', 0x40)
#define NVME_IOCTL_ADMIN_CMD	_IOWR('N', 0x41, struct nvme_admin_cmd)

struct nvme_passthru_cmd {
    __u8	opcode;
    __u8	flags;
    __u16	rsvd1;
    __u32	nsid;
    __u32	cdw2;
    __u32	cdw3;
    __u64	metadata;
    __u64	addr;
    __u32	metadata_len;
    __u32	data_len;
    __u32	cdw10;
    __u32	cdw11;
    __u32	cdw12;
    __u32	cdw13;
    __u32	cdw14;
    __u32	cdw15;
    __u32	timeout_ms;
    __u32	result;
};

enum nvme_admin_opcode {

    nvme_admin_identify      = 0x06,

};

#endif // NVME_UTIL_H
