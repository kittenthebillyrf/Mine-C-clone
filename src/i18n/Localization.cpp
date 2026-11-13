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
            {"menu.singleplayer", "Singleplayer"},
            {"menu.multiplayer", "Multiplayer"},
            {"menu.options", "Options"},
            {"menu.quit", "Quit Game"},
            {"menu.back", "Back"},
            {"pause.continue", "Continue"},
            {"pause.settings", "Settings"},
            {"pause.save", "Save World"},
            {"pause.save_return", "Save and Return to Title"},
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
            {"help.languages", "Languages: en (English), ru (Русский), es (Español), de (Deutsch), fr (Français)"},
            {"error.output", "Error: cannot write output file: "},
            {"terrain.generated", "Generated {size}x{size} terrain, seed {seed} -> {output}"},
            {"language.unknown", "Unknown language '{code}', using English."}
        }},
        {"ru", {
            {"app.title", "Mine C++ Clone — прототип генерации ландшафта"},
            {"menu.singleplayer", "Одиночная игра"},
            {"menu.multiplayer", "Сетевая игра"},
            {"menu.options", "Настройки"},
            {"menu.quit", "Выйти из игры"},
            {"menu.back", "Назад"},
            {"pause.continue", "Продолжить"},
            {"pause.settings", "Настройки"},
            {"pause.save", "Сохранить мир"},
            {"pause.save_return", "Сохранить и выйти в меню"},
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
            {"help.languages", "Языки: en (English), ru (Русский), es (Español), de (Deutsch), fr (Français)"},
            {"error.output", "Ошибка: не удалось записать файл: "},
            {"terrain.generated", "Создан ландшафт {size}x{size}, seed {seed} -> {output}"},
            {"language.unknown", "Неизвестный язык '{code}', используется английский."}
        }},
        {"es", {
            {"app.title", "Mine C++ Clone - prototipo de terreno"},
            {"menu.singleplayer", "Un jugador"},
            {"menu.multiplayer", "Multijugador"},
            {"menu.options", "Opciones"},
            {"menu.quit", "Salir del juego"},
            {"menu.back", "Volver"},
            {"pause.continue", "Continuar"},
            {"pause.settings", "Ajustes"},
            {"pause.save", "Guardar mundo"},
            {"pause.save_return", "Guardar y volver al menú"},
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
            {"help.languages", "Idiomas: en (English), ru (Русский), es (Español), de (Deutsch), fr (Français)"},
            {"error.output", "Error: no se puede escribir el archivo: "},
            {"terrain.generated", "Terreno {size}x{size} generado, semilla {seed} -> {output}"},
            {"language.unknown", "Idioma desconocido '{code}', se usará inglés."}
        }},
        {"de", {
            {"app.title", "Mine C++ Clone - Terrain-Prototyp"},
            {"menu.singleplayer", "Einzelspieler"},
            {"menu.multiplayer", "Mehrspieler"},
            {"menu.options", "Optionen"},
            {"menu.quit", "Spiel beenden"},
            {"menu.back", "Zurück"},
            {"pause.continue", "Fortsetzen"},
            {"pause.settings", "Einstellungen"},
            {"pause.save", "Welt speichern"},
            {"pause.save_return", "Speichern und zum Menü"},
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
            {"help.languages", "Sprachen: en (English), ru (Русский), es (Español), de (Deutsch), fr (Français)"},
            {"error.output", "Fehler: Ausgabedatei kann nicht geschrieben werden: "},
            {"terrain.generated", "Terrain {size}x{size} erstellt, Seed {seed} -> {output}"},
            {"language.unknown", "Unbekannte Sprache '{code}', Englisch wird verwendet."}
        }},
        {"fr", {
            {"app.title", "Mine C++ Clone - prototype de terrain"},
            {"menu.singleplayer", "Solo"},
            {"menu.multiplayer", "Multijoueur"},
            {"menu.options", "Options"},
            {"menu.quit", "Quitter le jeu"},
            {"menu.back", "Retour"},
            {"pause.continue", "Continuer"},
            {"pause.settings", "Paramètres"},
            {"pause.save", "Sauvegarder le monde"},
            {"pause.save_return", "Sauvegarder et revenir au menu"},
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
            {"help.languages", "Langues : en (English), ru (Русский), es (Español), de (Deutsch), fr (Français)"},
            {"error.output", "Erreur : impossible d'écrire le fichier : "},
            {"terrain.generated", "Terrain {size}x{size} généré, graine {seed} -> {output}"},
            {"language.unknown", "Langue inconnue « {code} », anglais utilisé."}
        }},
    };
    return data;
}
}
const std::vector<Language>& languages() {
    static const std::vector<Language> list{
        {"en", "English"}, {"ru", "Русский"}, {"es", "Español"},
        {"de", "Deutsch"}, {"fr", "Français"}
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
