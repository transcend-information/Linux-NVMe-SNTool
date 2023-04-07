# Linux-NVMe-SNTool

  **Build Guide**
  
  - **compile the executable file**
  
    ```$g++ -static nvme_sn_tool.cpp nvme_util.cpp -o nvme_sn_tool```
    
  - **use <chmod +x> for sure you have proper right to execute the executable file**

    ```$chmod +x nvme_sn_tool```\
    ```$sudo ./nvme_sn_tool /dev/nvmeX```
    
#
  **Usage**

  - **nvme_sn_tool \<device>**\
    show specific information of the device
