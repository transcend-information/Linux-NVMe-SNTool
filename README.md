# Linux-NVMe-SNTool

  **The sample code demonstrates that get the Model Name, FW version and Serial No. of Transcend NVMe SSD. For Crestron Developer Use. **

  **Build Guide**
  
  - **compile the executable file**
  
    ```$g++ -static nvme_sn_tool.cpp nvme_util.cpp -o nvme_sn_tool```
    
  - **use <chmod +x> for sure you have proper right to execute the executable file**

    ```$chmod +x nvme_sn_tool```\
#
  - **Usage**

  ```$sudo ./nvme_sn_tool /dev/nvmeX```
     show specific information of the NVMe device

#
  <img src="https://github.com/transcend-information/Linux-NVMe-SNTool-Crestron/blob/master/Screenshot.png" width=70% height=70%>
