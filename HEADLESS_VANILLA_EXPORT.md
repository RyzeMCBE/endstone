# Exportador headless de datos vanilla

Esta variante mantiene el extractor de datos de Endstone, pero añade un comando de consola que funciona en Linux sin GLFW, ImGui, OpenGL, Xvfb, VNC ni escritorio remoto.

## Comando

Después de que BDS muestre `Server started.`, ejecuta en la consola:

```text
exportvanilla
```

Alias:

```text
exportdata
```

La recopilación se ejecuta en el hilo principal de BDS y los archivos se escriben en:

```text
<directorio-del-servidor>/data
```

Se generan:

```text
biomes.json
block_palette.nbt
block_states.json
block_tags.json
block_types.json
creative_groups.json
creative_items.nbt
item_components.nbt
item_tags.json
items.json
recipes.json
```

El comando solo puede utilizarse desde la consola.

## Paquete de estructuras Jigsaw (ID 313)

El exportador guarda una copia **binaria exacta** del paquete JigsawStructureDataPacket
cuando BDS lo envia a un cliente. No interpreta estructuras internas de memoria de BDS
ni inventa NBT. El archivo contiene el **encabezado de red** (varuint) y todo el
payload original:

```text
jigsaw_structure_data_packet.bin
```

Para que el archivo este disponible:

1. Inicia el BDS compatible y espera a que el servidor indique que ha arrancado.
2. Conecta al menos un cliente Bedrock de version compatible y permite que BDS envie
   el paquete 313 (el paquete puede depender de la configuracion de worldgen del mundo).
3. Ejecuta `exportvanilla` de nuevo. Se guardaran los 11 archivos habituales y,
   si se capturo, tambien el paquete Jigsaw.

Si BDS todavia no ha emitido el paquete, `exportvanilla` registra una advertencia
y exporta los 11 archivos restantes; **no genera un paquete Jigsaw vacio**.

En el exportador con interfaz, el menu `File > Export > Export All` incluye Jigsaw
si ya se capturo, y `File > Export > Jigsaw Packet (313)` permite exportarlo por
separado. Esta ultima opcion se habilita solo despues de una captura.

Importante: este archivo binario **no es un NBT listo para importar**, sino
un paquete Bedrock de red. Debe decodificarse usando el codec correspondiente.
El limite de captura es 64 MiB para evitar retener indefinidamente un paquete
anormal. Si necesitas Jigsaw sin conectar un cliente, habra que validar primero
la ABI de JigsawStructureRegistry del BDS exacto; no debe reutilizarse el
offset 600 de BDS 1.26.50/1.26.51 sin esa validacion.

## Compatibilidad con BDS 1.26.60

Endstone oficial incorporo soporte para BDS 1.26.60 en la version 0.11.15
(2026-10-28). Este fork se basa aun en codigo anterior; antes de ejecutar el
servidor 1.26.60 con estos cambios se debe integrar la actualizacion oficial
y compilar/probar el fork. No se asume compatibilidad binaria solo porque
el exportador ha sido actualizado.


## Compilar en Ubuntu

El proyecto utiliza Clang 20, libc++ y Ninja. Desde la raíz del código fuente:

```bash
sudo apt-get update
sudo apt-get install -y build-essential lsb-release wget software-properties-common gnupg python3-venv
wget -q https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 20
sudo apt-get install -y libc++-20-dev libc++abi-20-dev clang-tools-20 ninja-build
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-20 200
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-20 200
```

Crea un entorno e instala el proyecto:

```bash
python3 -m venv .build-venv
source .build-venv/bin/activate
python -m pip install -U pip conan
python -m pip install -U . -C build-dir=./build
```

Para sustituir Endstone dentro de un bundle existente, activa el `.venv` de ese bundle y ejecuta la instalación desde esta carpeta de código fuente:

```bash
source /ruta/al/bundle/.venv/bin/activate
python -m pip install -U . -C build-dir=./build
```

Luego inicia el bundle normalmente con `start.sh`, espera `Server started.` y ejecuta `exportvanilla`.
