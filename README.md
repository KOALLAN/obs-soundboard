# OBS Soundboard — Community Adjustments

Free, unofficial community update of the original **OBS Soundboard** plugin. It keeps the original workflow and adds a few requested corrections and quality-of-life improvements.

## Features in this community update

- Individual volume from 0% to 150% for every sound.
- A settings button in the Soundboard dock for selecting the native, global OBS monitoring device.
- Automatic refresh of Soundboard monitoring whenever media playback starts.
- An option to hide embedded album covers and other artwork without muting the audio.
- Brazilian Portuguese and English translations.
- Portable Windows package compatible with the usual OBS `data` and `obs-plugins` folders.

The monitoring device selector uses the native OBS setting. Because that setting is global, changing it from the Soundboard also changes the monitoring device used by other OBS sources.

## Install on Windows

1. Close OBS Studio.
2. Extract the portable ZIP into the OBS Studio directory, merging the `data` and `obs-plugins` folders.
3. Open OBS Studio and use **Docks > Soundboard**.
4. Use the gear button in the Soundboard dock to select the monitoring device and configure cover artwork visibility.

Existing Soundboard 2.0.0 sounds, hotkeys, loop settings and dock layout are preserved. Sounds without a saved volume use 100%.

## Original project and credits

This repository is based on [cg2121/obs-soundboard](https://github.com/cg2121/obs-soundboard), created and maintained originally by [cg2121](https://github.com/cg2121). All credit for the original plugin and its architecture belongs to the original author and contributors.

This is not an official release from the original author. The community update is provided free of charge and remains free software under the same [GNU General Public License version 2](LICENSE). The original license and attribution are preserved.

Community adjustments are maintained at [KOALLAN/obs-soundboard](https://github.com/KOALLAN/obs-soundboard).

---

## Português (Brasil)

Esta é uma atualização comunitária gratuita e não oficial do **OBS Soundboard** original. Ela mantém o funcionamento original e acrescenta:

- volume individual de 0% a 150% para cada som;
- seleção do dispositivo global de monitoramento do OBS pelo painel do Soundboard;
- renovação automática do monitoramento ao iniciar cada reprodução;
- opção para ocultar capas e imagens incorporadas sem remover o áudio;
- traduções em português brasileiro e inglês;
- pacote portátil para Windows com as pastas `data` e `obs-plugins`.

O projeto original é [cg2121/obs-soundboard](https://github.com/cg2121/obs-soundboard), de autoria de [cg2121](https://github.com/cg2121). Esta modificação não é uma versão oficial do autor original. Ela é distribuída gratuitamente, preservando os créditos e a licença GNU GPL v2 do projeto original.

Os ajustes comunitários são mantidos em [KOALLAN/obs-soundboard](https://github.com/KOALLAN/obs-soundboard).
