# OBS Soundboard — Community Update

Free and unofficial community update of the original **OBS Soundboard** plugin. It keeps the original one-click soundboard workflow while adding per-sound controls, a customizable square grid and safer access to OBS's native monitoring settings.

This fork is based on [cg2121/obs-soundboard](https://github.com/cg2121/obs-soundboard). It is not an official release from the original author.

## Features

- One-click playback from a dock inside OBS Studio.
- Per-sound volume from 0% to 150%.
- Square sound cards with centered names.
- An optional PNG, JPG, WebP or BMP image for every sound card.
- Per-sound hotkeys, looping, renaming, duplication and drag-to-reorder.
- Grid and list layouts.
- Native OBS monitoring-device selection from the Soundboard settings.
- Stable monitoring: the audio route is initialized while idle and is not restarted whenever playback begins.
- A manual **Restart monitoring** action for device recovery.
- An option to hide embedded album artwork from the stream or recording without changing the audio.
- English and Brazilian Portuguese localization.
- Portable Windows package using the standard OBS `data` and `obs-plugins` folders.

## Installation on Windows

1. Download the latest portable ZIP from this fork's [Releases](../../releases) page.
2. Close OBS Studio.
3. Extract the ZIP into the OBS Studio installation directory, merging the `data` and `obs-plugins` folders.
4. Open OBS Studio and enable **Docks > Soundboard** if the dock is not already visible.

Existing Soundboard 2.0.0 and 2.0.1 entries are preserved. Sounds without a saved volume use 100%, and sounds without an image remain text-only.

## Usage

The toolbar at the bottom of the Soundboard dock provides the main actions:

| Button | Action |
| --- | --- |
| **+** | Add a sound, choose its audio file, volume, loop state and optional card image. |
| **−** | Remove the selected sound. |
| **Pencil** | Edit the selected sound and its card image. |
| **Gear** | Configure native monitoring and embedded artwork visibility. |

Right-click inside the dock to rename or duplicate a sound, open the source filters, or switch between list and grid layouts. Hotkeys can be assigned under **Settings > Hotkeys** in OBS.

Card images are referenced by their file path. If an image is moved or deleted, the sound remains available and its card falls back to text-only display.

## Audio monitoring

The monitoring device selector uses OBS Studio's native global setting. Changing it from the Soundboard also changes the device used by other monitored OBS sources.

Version 2.0.2 keeps the monitoring route stable during playback. It no longer disables and enables monitoring every time a sound starts, which avoids rebuilding the audio route at the beginning of the file. Use **Restart monitoring** only when a device change or driver recovery requires it.

To avoid duplicated audio, do not capture the same monitoring device through Desktop Audio while also using **Monitor and Output**, unless that routing is intentional.

## Building from source

This project follows the standard [OBS plugin template](https://github.com/obsproject/obs-plugintemplate) CMake workflow. Dependencies and versions are declared in `buildspec.json`; the GitHub Actions workflows build and package the supported platforms.

## Credits

- Original plugin and architecture: [cg2121/obs-soundboard](https://github.com/cg2121/obs-soundboard), created and maintained originally by [cg2121](https://github.com/cg2121).
- Community adjustments: [KOALLAN/obs-soundboard](https://github.com/KOALLAN/obs-soundboard).
- Built with the official [OBS plugin template](https://github.com/obsproject/obs-plugintemplate).

All credit for the original plugin belongs to its original author and contributors. This community fork only adds adjustments and corrections on top of that work.

## License and cost

This community update is free of charge and remains free software under the same [GNU General Public License version 2](LICENSE) used by the original project. The original license and attribution are preserved.

---

## Português (Brasil)

Esta é uma atualização comunitária gratuita e não oficial do **OBS Soundboard**, baseada no projeto original [cg2121/obs-soundboard](https://github.com/cg2121/obs-soundboard).

### Recursos desta versão

- volume individual de 0% a 150% para cada som;
- cartões quadrados com o nome centralizado;
- imagem opcional em PNG, JPG, WebP ou BMP para cada botão;
- atalhos, repetição, renomeação, duplicação e reorganização dos sons;
- modos de visualização em grade e lista;
- seleção do dispositivo global de monitoramento usando a função nativa do OBS;
- monitoramento estável, sem desligar e religar a rota quando cada som começa;
- botão manual para reiniciar o monitoramento quando necessário;
- opção para ocultar capas incorporadas sem alterar o áudio;
- interface em português brasileiro e inglês;
- pacote portátil para Windows.

### Instalação

1. Baixe o ZIP portátil mais recente na página de [Releases](../../releases).
2. Feche o OBS Studio.
3. Extraia o conteúdo na pasta do OBS, combinando as pastas `data` e `obs-plugins`.
4. Abra o OBS e habilite **Painéis > Soundboard**, caso o painel não esteja visível.

Os sons das versões anteriores são preservados. Sons antigos continuam com volume de 100% e sem imagem até serem editados.

As imagens ficam vinculadas ao caminho do arquivo escolhido. Se uma imagem for movida ou apagada, o áudio continua funcionando e o cartão volta a mostrar somente o texto.

### Monitoramento

O dispositivo escolhido é a configuração global nativa do OBS e também será usado pelas outras fontes monitoradas. Nesta versão, a rota é preparada enquanto a fonte está ociosa e permanece ativa durante as reproduções. O botão **Reiniciar monitoramento** fica disponível apenas para recuperação depois de mudanças de dispositivo ou problemas do driver.

### Créditos e licença

O plugin original foi criado por [cg2121](https://github.com/cg2121) e está disponível em [cg2121/obs-soundboard](https://github.com/cg2121/obs-soundboard). Esta modificação não é uma versão oficial do autor original.

Os ajustes comunitários são mantidos em [KOALLAN/obs-soundboard](https://github.com/KOALLAN/obs-soundboard). O projeto continua gratuito e licenciado sob a [GNU GPL v2](LICENSE), preservando os créditos e a licença original.
