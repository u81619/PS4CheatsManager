#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <orbis/Sysmodule.h>
#include <orbis/AudioOut.h>
#include <orbis/CommonDialog.h>
#include <orbis/Sysmodule.h>
#include <orbis/SystemService.h>

#include "cheats.h"
#include "util.h"
#include "common.h"
#include "orbisPad.h"

#include "menu.h"
#include "menu_gui.h"

#include "libfont.h"
#include "ttf_render.h"
#include "font-16x32.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

static int32_t audio = 0;

#define load_menu_texture(name, type) \
			if (!LoadMenuTexture(CHEATSMGR_APP_PATH "images/" #name "." #type , name##_##type##_index)) return 0;

app_config_t gcm_config = {
    .app_name = "CHTMGR",
    .app_ver = {0},
    .music = 1,
    .doSort = 1,
    .doAni = 1,
    .update = 1,
    .overwrite = 1,
    .user_id = 0,
    .url_cheats = GOLDCHEATS_URL,
    .url_patches = GOLDPATCH_URL,
    .url_plugins = GOLDPLUGINS_UPDATE_URL,
};

int close_app = 0;
int idle_time = 0;

png_texture * menu_textures;
SDL_Window* window;
SDL_Renderer* renderer;
uint32_t* texture_mem;
uint32_t* free_mem;

game_list_t hdd_cheats = {
	.icon_id = header_ico_cht_png_index,
	.title = "غش القرص الصلب",
    .list = NULL,
    .path = GOLDCHEATS_DATA_PATH,
    .ReadList = &ReadUserList,
    .ReadCodes = &ReadCodes,
    .UpdatePath = NULL,
};

game_list_t hdd_patches = {
    .icon_id = header_ico_cht_png_index,
    .title = "تصحيحات اللعبة",
    .list = NULL,
    .path = GOLDCHEATS_PATCH_PATH "xml/",
    .ReadList = &ReadPatchList,
    .ReadCodes = &ReadPatches,
    .UpdatePath = NULL,
};

game_list_t online_cheats = {
	.icon_id = header_ico_cht_png_index,
	.title = "غش عبر الإنترنت",
    .list = NULL,
    .path = ONLINE_URL,
    .ReadList = &ReadOnlineList,
    .ReadCodes = &ReadOnlineCodes,
    .UpdatePath = NULL,
};

game_list_t update_cheats = {
    .icon_id = header_ico_xmb_png_index,
    .title = "تحديث الغش، التصحيحات والإضافات",
    .list = NULL,
    .path = "",
    .ReadList = &ReadBackupList,
    .ReadCodes = &ReadBackupCodes,
    .UpdatePath = NULL,
};

static const char* get_button_prompts(int menu_id)
{
	const char* prompt = NULL;

	switch (menu_id)
	{
		case MENU_PATCH_VIEW:
		case MENU_CREDITS:
		case MENU_SAVE_DETAILS:
			prompt = "\x13 رجوع";
			break;

		case MENU_SETTINGS:
		case MENU_CODE_OPTIONS:
			prompt = "\x10 اختيار    \x13 رجوع";
			break;

		case MENU_UPDATE_CHEATS:
			prompt = "\x10 اختيار    \x13 رجوع    \x11 تحديث";
			break;

		case MENU_HDD_CHEATS:
		case MENU_HDD_PATCHES:
		case MENU_ONLINE_DB:
			prompt = "\x10 اختيار    \x13 رجوع    \x12 تصفية    \x11 تحديث";
			break;

		case MENU_PATCHES:
			prompt = "\x10 اختيار    \x12 عرض الكود    \x13 رجوع";
			break;

		case MENU_MAIN_SCREEN:
		default:
			prompt = "";
			break;
	}

	return prompt;
}

static int initPad(void)
{
	if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_PAD) < 0)
		return 0;

	if (orbisPadInit() < 0)
	{
		LOG("[خطأ] فشل تهيئة مكتبة لوحة التحكم!");
		return 0;
	}

	gcm_config.user_id = orbisPadGetConf()->userId;

	return 1;
}

static int LoadTextures_Menu(void)
{
	texture_mem = malloc(256 * 32 * 32 * 4);
	menu_textures = (png_texture *)calloc(TOTAL_MENU_TEXTURES, sizeof(png_texture));
	
	if(!texture_mem || !menu_textures)
		return 0;
	
	ResetFont();
	free_mem = (u32 *) AddFontFromBitmapArray((u8 *) console_font_16x32, (u8 *) texture_mem, 0, 0xFF, 16, 32, 1, BIT7_FIRST_PIXEL);
	
	if (TTFLoadFont(0, "/preinst/common/font/DFHEI5-SONY.ttf", NULL, 0) != SUCCESS ||
		TTFLoadFont(1, "/system_ex/app/NPXS20113/bdjstack/lib/fonts/SCE-PS3-RD-R-LATIN.TTF", NULL, 0) != SUCCESS)
		return 0;

	free_mem = (u32*) init_ttf_table((u8*) free_mem);
	set_ttf_window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WIN_SKIP_LF);
	
	load_menu_texture(bgimg, png);
	load_menu_texture(bglist, png);
	load_menu_texture(cheat, png);
	load_menu_texture(pslogo, png);
	load_menu_texture(leon_luna, jpg);
	load_menu_texture(tag_shn, png);
	load_menu_texture(tag_mc4, png);
	load_menu_texture(tag_json, png);
	load_menu_texture(tag_lock, png);
	load_menu_texture(circle_error_dark, png);
	load_menu_texture(circle_error_light, png);
	load_menu_texture(circle_loading_bg, png);
	load_menu_texture(circle_loading_seek, png);
	load_menu_texture(edit_ico_add, png);
	load_menu_texture(edit_ico_del, png);
	load_menu_texture(edit_shadow, png);
	load_menu_texture(footer_ico_circle, png);
	load_menu_texture(footer_ico_cross, png);
	load_menu_texture(footer_ico_lt, png);
	load_menu_texture(footer_ico_rt, png);
	load_menu_texture(footer_ico_square, png);
	load_menu_texture(footer_ico_triangle, png);
	load_menu_texture(header_dot, png);
	load_menu_texture(header_ico_abt, png);
	load_menu_texture(header_ico_cht, png);
	load_menu_texture(header_ico_opt, png);
	load_menu_texture(header_ico_xmb, png);
	load_menu_texture(header_line, png);
	load_menu_texture(help, png);
	load_menu_texture(mark_arrow, png);
	load_menu_texture(mark_line, png);
	load_menu_texture(mark_line_nowork, png);
	load_menu_texture(opt_off, png);
	load_menu_texture(opt_on, png);
	load_menu_texture(scroll_bg, png);
	load_menu_texture(scroll_lock, png);
	load_menu_texture(titlescr_ico_abt, png);
	load_menu_texture(titlescr_ico_cht, png);
	load_menu_texture(titlescr_ico_pat, png);
	load_menu_texture(titlescr_ico_net, png);
	load_menu_texture(titlescr_ico_opt, png);
	load_menu_texture(titlescr_ico_xmb, png);
	load_menu_texture(titlescr_logo, png);

    return 1;

u32 tBytes = free_mem - texture_mem;
LOG("LoadTextures_Menu() :: تم تخصيص %db (%.02fkb, %.02fmb) للنسائج", tBytes, tBytes / (float)1024, tBytes / (float)(1024 * 1024));
return 1;
}

static int LoadSounds(void* data)
{
	uint8_t* play_audio = data;
	int32_t sOffs = 0;
	drmp3 wav;

	if (!drmp3_init_file(&wav, CHEATSMGR_APP_PATH "audio/background_music.mp3", NULL))
	{
		LOG("[خطأ] فشل فك ترميز ملف الصوت");
		return -1;
	}

	size_t sampleCount = drmp3_get_pcm_frame_count(&wav) * wav.channels;
	drmp3_int16 *pSampleData = (drmp3_int16 *)malloc(sampleCount * sizeof(uint16_t));

	drmp3_read_pcm_frames_s16(&wav, drmp3_get_pcm_frame_count(&wav), pSampleData);

	while (!close_app)
	{
		if (*play_audio == 0)
		{
			usleep(0x1000);
			continue;
		}

		sceAudioOutOutput(audio, NULL);

		if (sceAudioOutOutput(audio, pSampleData + sOffs) < 0)
		{
			LOG("فشل إخراج الصوت");
			return -1;
		}

		sOffs += 256 * 2;

		if (sOffs >= sampleCount)
			sOffs = 0;
	}

	free(pSampleData);
	drmp3_uninit(&wav);

	return 0;
}

static void registerSpecialChars(void)
{
	RegisterSpecialCharacter(orbisPadGetConf()->crossButtonOK ? CHAR_BTN_X : CHAR_BTN_O, 0, 1.0, &menu_textures[footer_ico_cross_png_index]);
	RegisterSpecialCharacter(CHAR_BTN_S, 0, 1.0, &menu_textures[footer_ico_square_png_index]);
	RegisterSpecialCharacter(CHAR_BTN_T, 0, 1.0, &menu_textures[footer_ico_triangle_png_index]);
	RegisterSpecialCharacter(orbisPadGetConf()->crossButtonOK ? CHAR_BTN_O : CHAR_BTN_X, 0, 1.0, &menu_textures[footer_ico_circle_png_index]);

	RegisterSpecialCharacter(CHAR_TAG_MC4, 0, 1.0, &menu_textures[tag_mc4_png_index]);
	RegisterSpecialCharacter(CHAR_TAG_SHN, 0, 1.0, &menu_textures[tag_shn_png_index]);
	RegisterSpecialCharacter(CHAR_TAG_JSON, 0, 1.0, &menu_textures[tag_json_png_index]);
	RegisterSpecialCharacter(CHAR_TAG_LOCKED, 0, 1.0, &menu_textures[tag_lock_png_index]);
}

static void helpFooter(int id)
{
	u8 alpha = 0xFF;

	if (gcm_config.doAni && orbisPadGetConf()->idle > 0x100)
	{
		int dec = (orbisPadGetConf()->idle - 0x100) * 2;
		alpha = (dec > alpha) ? 0 : (alpha - dec);
	}

	SetFontSize(APP_FONT_SIZE_DESCRIPTION);
	SetFontAlign(FONT_ALIGN_SCREEN_CENTER);
	SetFontColor(APP_FONT_MENU_COLOR | alpha, 0);
	DrawString(0, SCREEN_HEIGHT - 94, get_button_prompts(id));
	SetFontAlign(FONT_ALIGN_LEFT);
}

static void terminate(void)
{
	LOG("الخروج...");

	terminate_jbc();
	sceSystemServiceLoadExec("exit", NULL);
}

static int initInternal(void)
{
    int ret = sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE);
    if (ret != SUCCESS) {
        LOG("فشل تحميل الموديول: SYSTEM_SERVICE (0x%08x)\n", ret);
        return 0;
    }

    ret = sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE);
    if (ret != SUCCESS) {
        LOG("فشل تحميل الموديول: USER_SERVICE (0x%08x)\n", ret);
        return 0;
    }

    ret = sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SAVE_DATA);
    if (ret != SUCCESS) {
        LOG("فشل تحميل الموديول: SAVE_DATA (0x%08x)\n", ret);
        return 0;
    }

    return 1;
}

s32 main(s32 argc, const char* argv[])
{
#ifdef DEBUG_ENABLE_LOG
	uint32_t lastFrameTicks  = 0;
	uint32_t startFrameTicks = 0;
	uint32_t deltaFrameTicks = 0;

	dbglogger_init();
#endif

	LOG("تهيئة SDL");

	if (SDL_Init(SDL_INIT_VIDEO) != SUCCESS)
	{
		LOG("فشل تهيئة SDL: %s", SDL_GetError());
		return (-1);
	}

	initInternal();
	http_init();
	initPad();

	if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_AUDIOOUT) < 0 ||
		sceAudioOutInit() != SUCCESS)
	{
		LOG("[خطأ] فشل تهيئة إخراج الصوت");
		return (-1);
	}

	audio = sceAudioOutOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM, ORBIS_AUDIO_OUT_PORT_TYPE_MAIN, 0, 256, 48000, ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);

	if (audio <= 0)
	{
		LOG("[خطأ] فشل فتح الصوت على المنفذ الرئيسي");
		return audio;
	}

	LOG("إنشاء نافذة");
	window = SDL_CreateWindow("main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	if (!window) {
		LOG("SDL_CreateWindow: %s", SDL_GetError());
		return (-1);
	}

	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		LOG("SDL_CreateRenderer: %s", SDL_GetError());
		return (-1);
	}

	if (!initialize_jbc())
		terminate();

	mkdirs(GOLDCHEATS_DATA_PATH);
	mkdirs(CHEATSMGR_LOCAL_CACHE);
	mkdirs(GOLDCHEATS_PATCH_PATH "settings/");
	
	if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL) < 0)
	{
		LOG("فشل تحميل freetype!");
		return (-1);
	}

	if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG) < 0 ||
		sceSysmoduleLoadModule(ORBIS_SYSMODULE_IME_DIALOG) < 0)
	{
		LOG("فشل تحميل Dialog!");
		return (-1);
	}

	if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_COMMON_DIALOG) < 0 ||
		sceCommonDialogInitialize() < 0)
	{
		LOG("فشل تهيئة CommonDialog!");
		return (-1);
	}

	atexit(terminate);
	
	if (!LoadTextures_Menu())
	{
		LOG("فشل تحميل نسيجات القائمة!");
		return (-1);
	}

#ifndef DEBUG_ENABLE_LOG
	drawSplashLogo(1);
#endif

	load_app_settings(&gcm_config);

	if (strncmp(gcm_config.app_ver, CHEATSMGR_VERSION, sizeof(gcm_config.app_ver)) != 0)
	{
		if (gcm_config.overwrite && extract_zip(CHEATSMGR_APP_PATH "misc/" LOCAL_TEMP_ZIP, GOLDHEN_PATH "/"))
		{
			char *cheat_ver = readTextFile(GOLDCHEATS_DATA_PATH "misc/cheat_ver.txt", NULL);
			char *patch_ver = readTextFile(GOLDCHEATS_PATCH_PATH "misc/patch_ver.txt", NULL);
			show_message("تم تثبيت بيانات التطبيق المحلية بنجاح:\n\n- %s- %s", cheat_ver, patch_ver);
			free(cheat_ver);
			free(patch_ver);
		}


		strncpy(gcm_config.app_ver, CHEATSMGR_VERSION, sizeof(gcm_config.app_ver));
save_app_settings(&gcm_config);

SetExtraSpace(-1);
SetCurrentFont(font_console_regular);

registerSpecialChars();
initMenuOptions();
#ifndef DEBUG_ENABLE_LOG
drawSplashLogo(-1);
#endif
SDL_DestroyTexture(menu_textures[pslogo_png_index].texture);

update_callback(!gcm_config.update);

SDL_CreateThread(&LoadSounds, "audio_thread", &gcm_config.music);
#ifndef DEBUG_ENABLE_LOG
Draw_MainMenu_Ani();
#endif

while (!close_app)
{
#ifdef DEBUG_ENABLE_LOG
    startFrameTicks = SDL_GetTicks();
    deltaFrameTicks = startFrameTicks - lastFrameTicks;
    lastFrameTicks  = startFrameTicks;
#endif
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
	SDL_RenderClear(renderer);
	orbisPadUpdate();
	drawScene();

	helpFooter((last_menu_id[menu_id] == MENU_UPDATE_CHEATS) ? MENU_CODE_OPTIONS : menu_id);

#ifdef DEBUG_ENABLE_LOG
	SetFontColor(APP_FONT_COLOR | 0xFF, 0);
	DrawFormatString(50, 960, "FPS: %d", (1000 / deltaFrameTicks));
#endif
	SDL_RenderPresent(renderer);
}

#ifndef DEBUG_ENABLE_LOG
if (gcm_config.doAni)
	drawEndLogo();
#endif

SDL_DestroyRenderer(renderer);
SDL_DestroyWindow(window);
SDL_Quit();
http_end();
orbisPadFinish();
return 0;
}
