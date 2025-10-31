# Password Vault

A lightweight, open-source password manager built in **C++17** with **ImGui** and **OpenGL** for the UI and **AES-256-GCM encryption** for secure storage.

The goal of this project is to provide a simple, fully local password manager that demonstrates modern C++ development, encryption, and GUI design.  
All data is stored in a single encrypted file under your **LocalAppData** folder.

---

## Features

- **Master Password Protection**  
  Set your own master password when first running the app. This password is required to unlock the vault.

- **AES-256-GCM Encryption**  
  All data is encrypted with OpenSSL (AES-256-GCM with PBKDF2 key derivation and a unique salt/IV for each vault).

- **Modern UI**  
  Built using ImGui + GLFW + OpenGL, styled with rounded corners and dark mode.

- **Password Management**
  - Add entries with `Website`, `Username/Email`, and `Password`.
  - Passwords are hidden by default (`********`) and can be toggled with an show button.
  - Copy passwords to clipboard with one click.

- **Auto Save**  
  Every new entry is automatically saved to the encrypted vault.

- **Check Password**
  - Check the strength of a password.
  - Generate a random strong password and copy it.

- **Search**
  Search up instantly and easily the password you need.

- **LocalAppData Storage**  
  Vault file (`vault.dat`) is saved under:

C:\Users<You>\AppData\Local\PasswordVault\


---

## Screenshots

<p align="center">
<!-- Replace with actual screenshots -->
<img src="https://i.imgur.com/m9O8Q4B.png" width="500" alt="Login Screen">
<br><em>Login / Create Master Password</em>
</p>

<p align="center">
<img src="https://i.imgur.com/loSyu9u.png" width="700" alt="Vault Entries">
<br><em>Vault entries with hidden passwords and copy/show/delete buttons</em>
</p>

---

## How It Works

1. **First Run**  
 If no vault exists, you’ll be prompted to create a master password. A new encrypted vault file is created in LocalAppData.

2. **Unlocking the Vault**  
 On subsequent runs, you must enter your master password to unlock and decrypt the vault.

3. **Adding Entries**  
 Fill in the `Website`, `Username`, and `Password` fields and click **Add Entry**. The entry is encrypted and saved immediately.

4. **Viewing Entries**  
 Entries appear in a table:
 - Passwords are masked by default.
 - Use the **Show** button to reveal them temporarily.
 - Use the **Copy** button to copy a password to the clipboard.

5. **Saving & Closing**  
 Vaults are saved automatically. Closing the app will leave your encrypted vault file in LocalAppData.

---

## How to use fast and easy
1. Go to releases
2. Download the executable file
3. Run it!

**If you want to build it yourself follow the instructions below!**

---

## Build Instructions

### Prerequisites
- **C++17 compiler** (MSVC recommended)
- **vcpkg** or equivalent package manager
- Dependencies:
- OpenSSL (AES/PBKDF2 crypto)
- GLFW (window/input handling)
- ImGui (UI library)
- nlohmann/json (JSON serialization)

### Steps
1. Clone the repository:
 ```bash
 git clone https://github.com/yourusername/password-vault.git
 cd password-vault
```

2. Install dependencies with vcpkg:

```bash
vcpkg install openssl:x64-windows-static glfw3:x64-windows-static nlohmann-json:x64-windows
```


3. Open the project in Visual Studio.
Make sure to set:

C++ Language Standard → ISO C++17

Runtime Library → /MT for static linking (so no DLLs required).

Properties → C++ → General → Additional Include Libraries → Change the paths to **YOUR** paths depending on where you installed the source code and the libraries needed from vcpkg

Properties → Linker → General → Additional Include Libraries → Change the paths to **YOUR** paths depending on where you installed the source code and the libraries needed from vcpkg

Release Mode

4. Build and run.
The app window should launch with the login screen.

---

# Credits: https://github.com/aggeloskwn7

---

# If you face any issues contact me through the issues page on this Repository and I will be happy to help
