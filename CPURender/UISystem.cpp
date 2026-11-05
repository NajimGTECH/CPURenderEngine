#include "Systems.h"

// Draws the UI elements of every visible canvas, canvas after canvas.
// An element whose canvasEntity doesn't name a canvas is never reached, so it isn't drawn.

void systems::draw_ui_images(EntityManager& em, const ComponentIDs& ids)
{
    for (int canvasEntity : em.fetch_all_entities(ids.canvas))
    {
        const CanvasComponent& canvas = *static_cast<CanvasComponent*>(em.get_component(canvasEntity, ids.canvas));
        if (!canvas.visible) continue;

        for (int entity : em.fetch_all_entities(ids.image))
        {
            const ImageComponent& image = *static_cast<ImageComponent*>(em.get_component(entity, ids.image));
            if (image.canvasEntity == canvasEntity) ui::draw_image(image.image, canvas.canvas);
        }
    }
}

void systems::draw_ui_text(EntityManager& em, const ComponentIDs& ids, SDL_Renderer* renderer)
{
    for (int canvasEntity : em.fetch_all_entities(ids.canvas))
    {
        const CanvasComponent& canvas = *static_cast<CanvasComponent*>(em.get_component(canvasEntity, ids.canvas));
        if (!canvas.visible) continue;

        for (int entity : em.fetch_all_entities(ids.text))
        {
            const TextComponent& text = *static_cast<TextComponent*>(em.get_component(entity, ids.text));
            if (text.canvasEntity == canvasEntity) ui::draw_text(text.text, canvas.canvas, renderer);
        }
    }
}
