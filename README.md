# RDT 3.0 Laser Communication with NUC100

A Nuvoton NUC100 firmware prototype for laser text communication using stop-and-wait framing, CRC checks, alternating sequence bits, and ACK/NACK responses.

[繁體中文](#繁體中文) · [English](#english)

## 繁體中文

### 專案簡介

本專案以 Nuvoton NUC100 系列微控制器實作 RDT 3.0 雷射通訊。發送端從 UART 接收文字，透過 GPIO 輸出位元訊號；接收端以 ADC 取樣並解碼，再透過回傳通道送出 ACK/NACK。發送端收到 NACK 時會重傳目前字元。

目前 ACK 判斷使用簡單 XOR 條件；若回傳位元翻轉，NACK 有機會被誤認為 ACK。這是原始韌體的協定限制，後續可加入具完整性保護的 ACK 封包改善。

### 封包流程

1. 傳送起始樣式 `0xAA`，位元順序為 LSB first。
2. 傳送一個長度位元組及其 CRC。
3. 每個資料字元以 7-bit ASCII 加上交替序號位元組成，再附上 CRC-8。資料與 CRC 均以 LSB first 傳送。
4. 接收端檢查序號及 CRC，並回傳 ACK/NACK（程式使用 `0xAA` / `0xAB` 交替樣式）。發送端收到 NACK 時重傳該字元。
5. 發送端以 `0x0D` 及其 CRC 結束訊息。

CRC 查表由程式產生，使用反射多項式 `0x8C`。

### 硬體與韌體設定

| 項目 | 程式中的設定 |
|---|---|
| MCU | Nuvoton NUC100 系列 |
| PLL 核心時脈 | 50 MHz |
| UART0 | 115200 baud |
| 發送端 Timer0 | 20 Hz |
| 接收端 Timer0 | 15 Hz |
| ADC1 輸入 | PA1 |
| ADC 判斷門檻 | 發送端 2048；接收端 1508 |
| 位元訊號 GPIO | PC13 |

上述 ADC 門檻與 Timer 頻率是原始程式中的常數。實際使用前應依開發板、雷射發射/接收電路與量測結果確認。ACK/NACK 需要回傳訊號路徑；本 repo 沒有附雷射驅動、光接收電路或完整接線圖。

### 建置與執行

此 repo 目前只包含 `sender.c` 和 `receiver.c`，沒有 IDE 專案、啟動檔、Nuvoton BSP 或一鍵建置腳本。程式使用 `NUC100Series.h` 及 NUC100 周邊函式庫，因此需要先準備相符版本的 Nuvoton NUC100 SDK/範例專案。

1. 建立兩個 NUC100 韌體目標，分別加入 `sender.c` 與 `receiver.c`。
2. 加入 SDK 所需的裝置標頭、啟動程式、系統設定與周邊驅動，並依實際電路設定 GPIO、ADC 與 Timer。
3. 將韌體燒錄至發送端與接收端開發板，並確認雙向光訊號路徑及 ADC 門檻。
4. 以 115200 baud 連接發送端 UART0，輸入文字並按 Enter；接收端 UART0 會輸出解碼與 CRC/ACK 狀態。

### 原始展示

[YouTube 展示影片](https://youtu.be/XgLk0znPfP4)

## English

### Overview

This project is a Nuvoton NUC100 firmware prototype for laser text communication. The sender accepts text over UART and emits bit signals through a GPIO. The receiver samples the incoming signal with its ADC, decodes the message, and returns ACK/NACK responses. The sender retransmits the current character after a NACK.

The current ACK check uses a simple XOR condition; a flipped response bit may make a NACK look like an ACK. This is a limitation of the original firmware. A framed ACK with integrity protection would make the response safer.

### Packet flow

1. Send the start pattern `0xAA`, least-significant bit first.
2. Send one length byte followed by its CRC.
3. Encode each character as 7-bit ASCII plus an alternating sequence bit, followed by a CRC-8. Data and CRC bits are sent least-significant bit first.
4. The receiver checks the sequence bit and CRC, then returns an ACK/NACK response using alternating `0xAA` / `0xAB` patterns. The sender retries that character after a NACK.
5. The sender closes the message with `0x0D` and its CRC.

The CRC lookup table is generated in firmware using reflected polynomial `0x8C`.

### Hardware and firmware settings

| Item | Source setting |
|---|---|
| MCU | Nuvoton NUC100 series |
| PLL core clock | 50 MHz |
| UART0 | 115200 baud |
| Sender Timer0 | 20 Hz |
| Receiver Timer0 | 15 Hz |
| ADC1 input | PA1 |
| ADC thresholds | Sender: 2048; receiver: 1508 |
| Bit-signal GPIO | PC13 |

The ADC thresholds and timer frequencies are constants in the original source. Confirm them against the boards, laser transmitter/receiver circuit, and measured signal timing before use. ACK/NACK requires a return signal path. This repository does not include the laser driver, optical receiver circuit, or a complete wiring diagram.

### Build and run

This repository currently contains only `sender.c` and `receiver.c`; it does not include IDE project files, startup code, the Nuvoton BSP, or a one-command build script. The firmware includes `NUC100Series.h` and uses NUC100 peripheral drivers, so a matching Nuvoton NUC100 SDK or example project is required.

1. Create two NUC100 firmware targets, one for `sender.c` and one for `receiver.c`.
2. Add the device headers, startup code, system configuration, and peripheral drivers from the SDK. Configure GPIO, ADC, and timers for the actual circuit.
3. Flash the sender and receiver boards, then verify the bidirectional optical path and ADC thresholds.
4. Connect to the sender's UART0 at 115200 baud, enter a message, and press Enter. The receiver's UART0 reports decoded data and CRC/ACK status.

### Demo

[YouTube demonstration](https://youtu.be/XgLk0znPfP4)
