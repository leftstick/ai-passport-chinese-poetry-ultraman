idf_component_register(
    SRCS "main.c"
         "app/adventure_app.c"
         "core/adventure.c"
         "core/collection.c"
         "core/poem_challenge.c"
         "core/poem_question.c"
         "core/poem_text.c"
         "audio/audio_player.c"
         "audio/poetry_audio.c"
         "assets/poem_library.c"
         "assets/poetry_art.c"
         "fonts/app_font_12.c"
         "fonts/app_font_14.c"
         "fonts/app_font_20.c"
         "fonts/app_font_28.c"
    INCLUDE_DIRS "." "app" "core" "audio" "assets"
    REQUIRES bsp esp_timer nvs_flash
)
