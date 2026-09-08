# OBS Soundboard — Community Update

Free and unofficial community update of the original **OBS Soundboard** plugin. It keeps the original one-click soundboard workflow while adding per-sound controls, a customizable square grid and safer access to OBS's native monitoring settings.

This fork is based on [cg2121/obs-soundboard](https://github.com/cg2121/obs-soundboard). It is not an official release from the original author.

## Features

- One-click playback from a dock inside OBS Studio.
- Rounded cards with a 2 px border in `#2B2E38`, turning green (`#22C55E`) only while that sound is playing.
- Responsive square cards that grow and shrink between configurable minimum and maximum sizes.
- Compact 4 px spacing with equal left and right margins around the centered grid.
- Per-sound volume from 0% to 150%.
- Configurable minimum and maximum card sizes (64–256 logical pixels), preserved when reopening OBS.
- Image layout: fit the whole image at the top or center-crop it to fill the button.
- Text at the top, center or bottom, horizontally centered.
- An optional PNG, JPG, WebP or BMP image for every sound card.
- Optional automatic cover matching: the newest image with the same base name as the audio is selected from its folder.
- One-click cover refresh for all sounds, plus an individual refresh action for each card.
- Missing audio is marked with a red border and can be located again or removed directly from the warning.
- Per-sound hotkeys, looping, renaming, duplication and drag-to-reorder.
- Grid and list layouts.
- Native OBS monitoring-device selection from the Soundboard settings.
- Stable monitoring: the audio route is initialized while idle and is not restarted whenever playback begins.
- A manual **Restart monitoring** action for device recovery.
- An option to hide embedded album artwork from the stream or recording without changing the audio.
- English and Brazilian Portuguese localization.
- Portable Windows package using the standard OBS `data` and `obs-plugins` folders.

## Installation on Windows

1. Download the latest portable ZIP from this fork's [Releases](https://github.com/KOALLAN/obs-soundboard/releases) page.
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
| **Circular arrow** | Find and refresh matching covers for all sounds. |
| **Gear** | Configure minimum and maximum button sizes, image layout, text position, automatic cover matching, native monitoring and embedded artwork visibility. |

Right-click a sound to refresh only its cover, rename, edit, duplicate or delete its button. Right-click inside the dock to open source filters or switch between list and grid layouts. Hotkeys can be assigned under **Settings > Hotkeys** in OBS.

Card images are referenced by their original file path and are never copied into the plugin folder. A manual cover remains selected until a requested automatic refresh finds a matching image. If no match exists, the current cover is preserved. When several supported images share the audio's base name, the most recently modified one is used.

### New in 2.0.10: automatic covers and missing-file recovery

Version 2.0.10 can match card covers without copying or caching images. Enable **Find a cover automatically when adding sounds** in Soundboard Settings to look beside every newly added local audio file. Enabling the option for the first time also refreshes existing cards. The circular-arrow toolbar action repeats the search for all sounds; **Refresh this button's cover** in the context menu updates only the selected sound.

Matching is case-insensitive and uses the complete filename without the final extension. PNG, JPG, JPEG, WebP and BMP are supported. If more than one matching image exists, the file with the newest modification time wins. A search that finds nothing leaves the current cover untouched, so manually chosen artwork remains stable until another matching refresh is explicitly requested.

If a saved local audio file has been moved or deleted, its card receives a red border. Clicking it shows **Directory changed** with actions to locate the audio again, delete the button or cancel. Reconnecting the file updates the stored path without recreating the sound. The per-card context menu also provides a clearly labelled **Delete button** action. These behaviors were confirmed by the user in the portable Windows build before publication.

### New in 2.0.9: deterministic full-row layout

Version 2.0.9 fixes the remaining empty card-sized area at the right side of the dock. The responsive size calculation was correct, but Qt's automatic `QListView` wrapping made a second, independent column decision and could still move the final card to the next row.

Grid cards are now positioned explicitly after their responsive size and column count are calculated. This guarantees the intended number of columns, preserves the compact 4 px gap and splits unused width equally between the left and right margins. Positions are recalculated after resizing, adding or reordering sounds. Selection, playback indication and drag-to-reorder remain available. The correction was verified in the portable Windows build before publication.

### New in 2.0.8: responsive and centered cards

Version 2.0.8 replaces the fixed card size with configurable minimum and maximum sizes. The grid chooses how many columns fit at the minimum size, then grows every square card evenly until the configured maximum is reached. The 4 px gap stays compact instead of absorbing unused width, and any remaining space is split equally between the left and right sides of the grid.

A one-pixel internal layout guard prevents the final column from wrapping at exact Windows DPI boundaries. Existing collections migrate to an 80 px minimum and a maximum of at least 160 px; both values can be changed under **Soundboard Settings** and are saved with the scene collection. The grid remains aligned to the top so controls do not move vertically as sounds are added.

### New in 2.0.7: DPI boundary correction

Version 2.0.7 removes the obsolete fixed layout allowance that could still prevent the final column from appearing under Windows display scaling. The adaptive card-size calculation introduced in 2.0.6 already provides the necessary safety at column boundaries, so the complete viewport width can now be used.

### New in 2.0.6: full-width grid rows

Version 2.0.6 counts the 4 px spacing only between adjacent cards, instead of reserving another gap after the final card. Near a column boundary, cards may render up to 3 logical pixels smaller than the configured size so an additional column fits while the minimum visible spacing is preserved. This removes the unused card-sized area at the right side of the dock.

### New in 2.0.5: reliable row filling

Version 2.0.5 accounts for the small internal layout allowance used by Qt when calculating grid columns. This prevents the last card from wrapping to the next row when it visibly fits in the remaining space, including docks displayed with Windows DPI scaling. Button size and the minimum 4 px spacing remain unchanged. Audio files dragged into the dock now use their filename without the extension as the button label.

### New in 2.0.4: rounded cards and playback indicator

Version 2.0.4 adds rounded corners and a fixed `#2B2E38` border. The currently playing sound has a green `#22C55E` border, including playback through hotkeys. Pausing, stopping, finishing or encountering a playback error returns the border to its normal color. Selecting another card does not move the playback indicator. Images are clipped to the rounded corners.

Horizontal spacing now adapts to the dock width, with a minimum 4 px gap. Cards stay at their configured size; rows keep their compact 4 px vertical spacing. A single column is centered. No additional audio-monitoring refresh timer is used.

### New in 2.0.3: persistent card layout

Open **Soundboard Settings** (gear) to set the square button size from 64 to 256 logical pixels. Cards wrap into as many columns as the dock fits, without stretching when OBS restores the panel. These settings are saved with the scene collection.

Choose **At the top** to show the entire image above the label, or **Fill the button** to cover the card without distorting the image (edges may be cropped). Text can be at the top, center or bottom. In top-image mode, that position refers to the area below the image; in fill mode, the label overlays the image on a translucent dark background for readability. Long names remain available in the tooltip.

These appearance settings apply to all grid buttons; list mode keeps its compact layout. Changing only appearance does not restart or change the monitoring route. Audio processing is unchanged. In-OBS playback and visual testing are left to the user.

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
- cantos arredondados e contorno de 2 px em `#2B2E38`, verde (`#22C55E`) enquanto o som toca;
- cartões quadrados responsivos, que aumentam ou diminuem entre os tamanhos mínimo e máximo configurados;
- distância compacta de 4 px entre os botões e margens iguais à esquerda e à direita da grade centralizada;
- tamanhos mínimo e máximo ajustáveis entre 64 e 256 pixels lógicos, preservados ao reabrir o OBS;
- imagem inteira na parte de cima ou preenchendo o botão com recorte central, sem distorção;
- texto em cima, no centro ou embaixo, sempre centralizado horizontalmente;
- imagem opcional em PNG, JPG, WebP ou BMP para cada botão;
- localização automática opcional da capa mais recente com o mesmo nome-base do áudio;
- atualização de capas para todos os sons ou individualmente em cada botão;
- contorno vermelho quando o áudio não é encontrado, com opções para localizá-lo novamente ou apagar o botão;
- atalhos, repetição, renomeação, duplicação e reorganização dos sons;
- modos de visualização em grade e lista;
- seleção do dispositivo global de monitoramento usando a função nativa do OBS;
- monitoramento estável, sem desligar e religar a rota quando cada som começa;
- botão manual para reiniciar o monitoramento quando necessário;
- opção para ocultar capas incorporadas sem alterar o áudio;
- interface em português brasileiro e inglês;
- pacote portátil para Windows.

### Instalação

1. Baixe o ZIP portátil mais recente na página de [Releases](https://github.com/KOALLAN/obs-soundboard/releases).
2. Feche o OBS Studio.
3. Extraia o conteúdo na pasta do OBS, combinando as pastas `data` e `obs-plugins`.
4. Abra o OBS e habilite **Painéis > Soundboard**, caso o painel não esteja visível.

Os sons das versões anteriores são preservados. Sons antigos continuam com volume de 100% e sem imagem até serem editados.

As imagens continuam no local original e ficam apenas vinculadas pelo caminho; nada é copiado para a pasta do plugin. Uma capa manual permanece até que uma atualização automática solicitada encontre outra imagem correspondente. Se não houver correspondência, a capa atual não é alterada. Havendo várias imagens compatíveis com o mesmo nome-base, vence a modificada mais recentemente.

### Novidades da 2.0.10: capas automáticas e recuperação de arquivos

A versão 2.0.10 localiza capas sem copiar imagens nem criar cache. Ative **Localizar capa automaticamente ao adicionar sons** nas Configurações do Soundboard para procurar uma imagem ao lado de cada novo arquivo de áudio local. Ao ativar a opção pela primeira vez, os botões existentes também são atualizados. O botão de seta circular repete a busca para todos os sons; **Atualizar capa deste botão**, no menu de contexto, altera somente o som selecionado.

A correspondência ignora maiúsculas e minúsculas e compara o nome completo sem a última extensão. São aceitos PNG, JPG, JPEG, WebP e BMP. Quando mais de uma imagem corresponde ao áudio, é utilizada a que foi modificada mais recentemente. Se a busca não encontrar nada, a capa atual é preservada; assim, uma imagem escolhida manualmente só muda quando uma nova atualização solicitada encontra uma correspondência.

Se um áudio local salvo for movido ou apagado, seu cartão recebe contorno vermelho. Ao clicar nele, a mensagem **Diretório alterado** oferece as opções de localizar novamente o áudio, apagar o botão ou cancelar. Ao reconectar o arquivo, o caminho salvo é atualizado sem recriar o som. O menu individual também passa a mostrar claramente a ação **Apagar botão**. Esses comportamentos foram confirmados pelo usuário no OBS portátil para Windows antes da publicação.

### Novidades da 2.0.9: preenchimento determinístico das linhas

A versão 2.0.9 corrige o espaço restante do tamanho de um cartão no lado direito do painel. O cálculo responsivo estava correto, mas a quebra automática do `QListView` do Qt fazia uma segunda decisão independente sobre as colunas e ainda podia empurrar o último botão para a linha seguinte.

Agora os cartões são posicionados explicitamente depois do cálculo do tamanho responsivo e da quantidade de colunas. Isso garante a quantidade esperada de botões por linha, preserva a distância compacta de 4 px e divide igualmente a largura restante entre as margens esquerda e direita. As posições são recalculadas ao redimensionar o painel, adicionar ou reorganizar sons. A seleção, o indicador de reprodução e o arrastar para reorganizar continuam disponíveis. A correção foi confirmada no OBS portátil para Windows antes da publicação.

### Novidades da 2.0.8: botões responsivos e centralizados

A versão 2.0.8 troca o tamanho fixo por dois controles: **tamanho mínimo** e **tamanho máximo**. A grade calcula quantas colunas cabem usando o mínimo e aumenta igualmente todos os cartões até atingir o máximo. A distância entre eles permanece compacta em 4 px; qualquer largura que ainda sobrar é dividida igualmente entre as laterais esquerda e direita.

Uma proteção interna de um pixel evita que a última coluna pule de linha exatamente nos limites criados pela escala de tela do Windows. Coleções existentes passam a usar mínimo de 80 px e máximo de pelo menos 160 px; os dois valores podem ser alterados nas **Configurações do Soundboard** e ficam salvos com a coleção de cenas. A grade continua encostada no topo para que os controles não mudem verticalmente quando novos sons forem adicionados.

### Novidades da 2.0.7: correção no limite com escala de tela

A versão 2.0.7 remove a antiga margem preventiva fixa que ainda podia impedir a última coluna quando o Windows usava escala de tela. O ajuste adaptável de até 3 pixels introduzido na 2.0.6 já protege os limites do layout, permitindo agora usar toda a largura real do painel.

### Novidades da 2.0.6: linhas ocupando toda a largura

A versão 2.0.6 conta os 4 px de distância somente entre botões vizinhos, sem reservar outro espaço depois do último botão. Perto do limite de uma coluna, os cartões podem aparecer até 3 pixels lógicos menores que o tamanho configurado para encaixar uma coluna adicional e preservar a distância mínima visível. Isso elimina a área vazia do tamanho de um botão no lado direito do painel.

### Novidades da 2.0.5: preenchimento correto das linhas

A versão 2.0.5 considera a pequena margem interna usada pelo Qt ao calcular as colunas. Isso evita que o último botão seja empurrado para a linha seguinte quando ainda cabe visualmente no espaço disponível, inclusive com a escala de tela do Windows. O tamanho escolhido e a distância mínima de 4 px permanecem iguais. Arquivos de áudio arrastados para o painel agora usam como nome do botão somente o nome do arquivo, sem a extensão.

### Novidades da 2.0.4: cantos arredondados e indicador de reprodução

A versão 2.0.4 adiciona cantos arredondados e contorno em `#2B2E38`. Somente o botão do som em reprodução fica com contorno verde (`#22C55E`), inclusive com atalhos. Ao pausar, parar, terminar ou ocorrer erro na reprodução, ele volta à cor normal. Selecionar outro botão não muda o indicador de reprodução. As imagens também respeitam os cantos arredondados.

O espaço horizontal entre os botões se ajusta à largura do painel, com distância mínima de 4 px. O tamanho escolhido dos botões é preservado; as linhas mantêm 4 px de distância vertical. Uma coluna fica centralizada. Esse indicador não acrescenta reinicializações nem um novo temporizador de monitoramento de áudio.

### Novidades da 2.0.3: aparência salva

Na **engrenagem das Configurações do Soundboard**, escolha o tamanho dos botões (64 a 256 pixels lógicos), a disposição da imagem e a posição do texto. O painel passa a organizar a quantidade de colunas sem esticar os botões ao abrir o OBS. As escolhas são salvas com a coleção de cenas.

Com a imagem **na parte de cima**, ela aparece inteira e o texto ocupa a área abaixo dela. No modo **preencher o botão**, a imagem cobre o quadrado sem distorção (com possível recorte nas bordas), e o texto fica sobre ela com um fundo escuro translúcido para facilitar a leitura. A posição em cima/centro/embaixo vale para a área de texto disponível. Nomes longos também podem ser lidos ao passar o mouse sobre o botão.

Esses ajustes valem para todos os botões da grade; o modo lista continua compacto. Alterar somente a aparência não reinicia nem muda a rota de monitoramento. O processamento de áudio não foi alterado. Os testes de reprodução e aparência dentro do OBS ficam com o usuário.

### Monitoramento

O dispositivo escolhido é a configuração global nativa do OBS e também será usado pelas outras fontes monitoradas. Nesta versão, a rota é preparada enquanto a fonte está ociosa e permanece ativa durante as reproduções. O botão **Reiniciar monitoramento** fica disponível apenas para recuperação depois de mudanças de dispositivo ou problemas do driver.

### Créditos e licença

O plugin original foi criado por [cg2121](https://github.com/cg2121) e está disponível em [cg2121/obs-soundboard](https://github.com/cg2121/obs-soundboard). Esta modificação não é uma versão oficial do autor original.

Os ajustes comunitários são mantidos em [KOALLAN/obs-soundboard](https://github.com/KOALLAN/obs-soundboard). O projeto continua gratuito e licenciado sob a [GNU GPL v2](LICENSE), preservando os créditos e a licença original.
