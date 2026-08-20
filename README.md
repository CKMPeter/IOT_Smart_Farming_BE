# This is the back end for the ESP 32 smart farming project
## Prerequisite
- vscode (with platform IO extension)
## Step to install onto your ESP
Note: there is a data folder_ which is where you store the build file from the Front End Repo.
- If there is no file in the data folder or you want to change/ update the design of the frontend please follow these step
  - Open Vscode in where you store your frontend or clone from [this Repo](https://github.com/CKMPeter/IOT_Smart_Farming_FE)
  Note: if clone, then you need to run `npm install` first, more detail please visit the link.
  - Run `npm run build`.
  - Copy the content of the build folder and put it into the data folder.
### To have a upload the code and have everything on your ESP
- First run `Upload Filesystem Image` (**IMPORTANT**: if you forgot this step, your sever will not have an UI).
- Second run `Upload and Monitor` for debugging
## How it work?
- The ESP has an intial setup state, if there is no wifi it will run in Access Point (AP) Mode
- While in the AP mode the ESP will present itself in your wifi setting as "ESP32-Setup" with the preset password is "12345678"
- After connecting to the Access Point an IP will be displayed on the screen, open it in the browser, and use your wifi credential to connect.
- If successfully given a wifi credential the ESP will run in Station (STA) Mode, and another IP, use it to access the dashboard.
## Other Detail
- Wiring diagram for this project
- <img width="1238" height="860" alt="image" src="https://github.com/user-attachments/assets/0874627c-88e0-4325-96cf-ebbe54cce892" />
