//
//Copyright(c) 2023 Perchik71 <email:timencevaleksej@gmail.com>
//

#pragma once

#include "vmmconfig.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

namespace voltek
{
	// Инициализация менеджера памяти.
	VOLTEK_MM_API void scalable_memory_manager_initialize();
	// Освобождение менеджера памяти.
	// Чисто символически, использовать её не рекомендуется.
	// Но если очень хочется, почему бы и нет.
	VOLTEK_MM_API void scalable_memory_manager_shutdown();
	// Выделение памяти нужного размера.
	// При ошибке вернёт nullptr, это если size равен 0 или более 4 гб.
	// Также вернёт nullptr если память физически кончилась.
	// Память всегда выровнена.
	VOLTEK_MM_API void* scalable_alloc(size_t size);
	// Выделение памяти нужного размера.
	// При ошибке вернёт nullptr, это если size равен 0 или более 4 гб.
	// Также вернёт nullptr если память физически кончилась.
	// Память всегда выровнена и обнулена.
	VOLTEK_MM_API void* scalable_calloc(size_t count, size_t size);
	// Выделение памяти нужного размера из прошлого указателя на память.
	// При ошибке вернёт nullptr, это если size равен 0 или более 4 гб.
	// Также вернёт nullptr если память физически кончилась.
	// Память всегда выровнена. Адрес памяти может быть изменён.
	VOLTEK_MM_API void* scalable_realloc(const void* ptr, size_t size);
	// Выделение памяти нужного размера из прошлого указателя на память.
	// При ошибке вернёт nullptr, это если size равен 0 или более 4 гб.
	// Также вернёт nullptr если память физически кончилась.
	// Память всегда выровнена. Адрес памяти может быть изменён.
	// Новый участок памяти всегда обнулён.
	VOLTEK_MM_API void* scalable_recalloc(const void* ptr, size_t count, size_t size);
	// Освобождает память выделенную под указатель.
	// Вернёт ложь, если произошла ошибка.
	VOLTEK_MM_API bool scalable_free(const void* ptr);
	// Возвращает размер памяти выделенной под указатель.
	// Вернёт 0 при ошибке, что значит, указатель на память не пренадлежит менеджеру.
	VOLTEK_MM_API size_t scalable_msize(const void* ptr);
}

#ifdef __cplusplus
} // __cplusplus defined.
#endif