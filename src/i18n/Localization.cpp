#include "Localization.hpp"

#include <array>
#include <string>
#include <unordered_map>

namespace mc::i18n {
namespace {
using Catalog = std::unordered_map<std::string, std::string>;
const std::unordered_map<std::string, Catalog>& catalogs() {
    static const std::unordered_map<std::string, Catalog> data{
        {"en", {
            {"app.title", "Mine C++ Clone - terrain prototype"},
            {"settings.title", "Settings"},
            {"settings.language", "Language"},
            {"settings.render_distance", "Render Distance"},
            {"settings.chunk_updates", "Chunk builds per frame"},
            {"settings.fov", "Field of view"},
            {"settings.sensitivity", "Mouse sensitivity"},
            {"settings.instructions", "Left/Right: change   Up/Down: select   B/Esc: back"},
            {"settings.chunks", "chunks"},
            {"settings.per_frame", "per frame"},
            {"help.usage", "Options: --size N --seed N --output FILE --lang CODE --help"},
            {"help.languages", "Languages: en (English), ru (Русский), es (Español), de (Deutsch), fr (Français), zh_CN (简体中文)"},
            {"error.output", "Error: cannot write output file: "},
            {"terrain.generated", "Generated {size}x{size} terrain, seed {seed} -> {output}"},
            {"language.unknown", "Unknown language '{code}', using English."}
        }},
        {"ru", {
            {"app.title", "Mine C++ Clone — прототип генерации ландшафта"},
            {"settings.title", "Настройки"},
            {"settings.language", "Язык"},
            {"settings.render_distance", "Дальность прорисовки"},
            {"settings.chunk_updates", "Перестройка чанков за кадр"},
            {"settings.fov", "Поле зрения"},
            {"settings.sensitivity", "Чувствительность мыши"},
            {"settings.instructions", "Влево/вправо: изменить   Вверх/вниз: выбор   B/Esc: назад"},
            {"settings.chunks", "чанков"},
            {"settings.per_frame", "за кадр"},
            {"help.usage", "Параметры: --size N --seed N --output ФАЙЛ --lang КОД --help"},
            {"help.languages", "Языки: en (English), ru (Русский), es (Español), de (Deutsch), fr (Français), zh_CN (简体中文)"},
            {"error.output", "Ошибка: не удалось записать файл: "},
            {"terrain.generated", "Создан ландшафт {size}x{size}, seed {seed} -> {output}"},
            {"language.unknown", "Неизвестный язык '{code}', используется английский."}
        }},
        {"es", {
            {"app.title", "Mine C++ Clone - prototipo de terreno"},
            {"settings.title", "Ajustes"},
            {"settings.language", "Idioma"},
            {"settings.render_distance", "Distancia de renderizado"},
            {"settings.chunk_updates", "Construcción de chunks por fotograma"},
            {"settings.fov", "Campo de visión"},
            {"settings.sensitivity", "Sensibilidad del ratón"},
            {"settings.instructions", "Izquierda/Derecha: cambiar   Arriba/Abajo: elegir   B/Esc: volver"},
            {"settings.chunks", "chunks"},
            {"settings.per_frame", "por fotograma"},
            {"help.usage", "Opciones: --size N --seed N --output ARCHIVO --lang CODIGO --help"},
            {"help.languages", "Idiomas: en (English), ru (Русский), es (Español), de (Deutsch), fr (Français), zh_CN (简体中文)"},
            {"error.output", "Error: no se puede escribir el archivo: "},
            {"terrain.generated", "Terreno {size}x{size} generado, semilla {seed} -> {output}"},
            {"language.unknown", "Idioma desconocido '{code}', se usará inglés."}
        }},
        {"de", {
            {"app.title", "Mine C++ Clone - Terrain-Prototyp"},
            {"settings.title", "Einstellungen"},
            {"settings.language", "Sprache"},
            {"settings.render_distance", "Sichtweite"},
            {"settings.chunk_updates", "Chunk-Neuaufbauten pro Bild"},
            {"settings.fov", "Sichtfeld"},
            {"settings.sensitivity", "Mausempfindlichkeit"},
            {"settings.instructions", "Links/Rechts: ändern   Hoch/Runter: wählen   B/Esc: zurück"},
            {"settings.chunks", "Chunks"},
            {"settings.per_frame", "pro Bild"},
            {"help.usage", "Optionen: --size N --seed N --output DATEI --lang CODE --help"},
            {"help.languages", "Sprachen: en (English), ru (Русский), es (Español), de (Deutsch), fr (Français), zh_CN (简体中文)"},
            {"error.output", "Fehler: Ausgabedatei kann nicht geschrieben werden: "},
            {"terrain.generated", "Terrain {size}x{size} erstellt, Seed {seed} -> {output}"},
            {"language.unknown", "Unbekannte Sprache '{code}', Englisch wird verwendet."}
        }},
        {"fr", {
            {"app.title", "Mine C++ Clone - prototype de terrain"},
            {"settings.title", "Paramètres"},
            {"settings.language", "Langue"},
            {"settings.render_distance", "Distance d'affichage"},
            {"settings.chunk_updates", "Reconstructions de chunks par image"},
            {"settings.fov", "Champ de vision"},
            {"settings.sensitivity", "Sensibilité de la souris"},
            {"settings.instructions", "Gauche/Droite : modifier   Haut/Bas : choisir   B/Esc : retour"},
            {"settings.chunks", "chunks"},
            {"settings.per_frame", "par image"},
            {"help.usage", "Options : --size N --seed N --output FICHIER --lang CODE --help"},
            {"help.languages", "Langues : en (English), ru (Русский), es (Español), de (Deutsch), fr (Français), zh_CN (简体中文)"},
            {"error.output", "Erreur : impossible d'écrire le fichier : "},
            {"terrain.generated", "Terrain {size}x{size} généré, graine {seed} -> {output}"},
            {"language.unknown", "Langue inconnue « {code} », anglais utilisé."}
        }},
        {"zh_CN", {
            {"app.title", "Mine C++ Clone - 地形生成原型"},
            {"settings.title", "设置"},
            {"settings.language", "语言"},
            {"settings.render_distance", "渲染距离"},
            {"settings.chunk_updates", "每帧区块重建数"},
            {"settings.fov", "视野"},
            {"settings.sensitivity", "鼠标灵敏度"},
            {"settings.instructions", "左右：更改   上下：选择   B/Esc：返回"},
            {"settings.chunks", "区块"},
            {"settings.per_frame", "每帧"},
            {"help.usage", "选项：--size N --seed N --output 文件 --lang 语言代码 --help"},
            {"help.languages", "语言：en (English), ru (Русский), es (Español), de (Deutsch), fr (Français), zh_CN (简体中文)"},
            {"error.output", "错误：无法写入输出文件："},
            {"terrain.generated", "已生成 {size}x{size} 地形，种子 {seed} -> {output}"},
            {"language.unknown", "未知语言“{code}”，将使用英语。"}
        }}
    };
    return data;
}
}
const std::vector<Language>& languages() {
    static const std::vector<Language> list{
        {"en", "English"}, {"ru", "Русский"}, {"es", "Español"},
        {"de", "Deutsch"}, {"fr", "Français"}, {"zh_CN", "简体中文"}
    };
    return list;
}
bool is_supported(std::string_view code) {
    return catalogs().contains(std::string(code));
}
std::string text(std::string_view key, std::string_view language) {
    const auto& all = catalogs();
    auto lang = all.find(std::string(language));
    if (lang == all.end()) lang = all.find("en");
    if (lang != all.end()) {
        auto value = lang->second.find(std::string(key));
        if (value != lang->second.end()) return value->second;
    }
    auto english = all.at("en").find(std::string(key));
    return english == all.at("en").end() ? std::string(key) : english->second;
}
}
