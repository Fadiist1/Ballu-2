// Подключение графической библиотеки SFML
#include <SFML/Graphics.hpp>
// Подключение аудиобиблиотеки SFML
#include <SFML/Audio.hpp>
// Заголовочный файл для управления камерой
#include "view.h"
// Стандартная библиотека ввода/вывода
#include <iostream>
// Библиотека для работы со строками
#include <sstream>
// Заголовочный файл с заданиями/миссиями
#include "mission.h"
// Заголовочный файл для работы с уровнями
#include "level.h"
// Библиотека для работы с динамическими массивами
#include <vector>
// Библиотека для работы со списками
#include <list>
// Библиотека для работы с XML-файлами
#include "TinyXML/tinyxml.h"
// Использование пространства имен SFML
using namespace sf;
#include <set>
#include <map>
#include <fstream>
#include <iomanip>

// Глобальные переменные для сохранения состояния
int savedLevel = 1;
int savedScore = 0;
int savedDeaths = 0;
bool item1Purchased = false;
bool item3Purchased = false;
std::string currentSkin = "pers.png";
std::map<int, std::set<std::pair<float, float>>> collectedCoinsMap;
//заблокировать движение при выходе из экрана, перезапуск уровня при смерти 

float soundEffectsVolume = 50.0f; // Громкость звуковых эффектов

//////////////////////////////////// ОСНОВНОЙ КЛАСС-РОДИТЕЛЬ //////////////////////////
class Entity {
public:
	// Вектор объектов карты для коллизий
	std::vector<Object> obj; // вектор объектов карты

	// Параметры движения и времени
	float dx; // скорость по оси X
	float dy; // скорость по оси Y
	float x;  // координата X объекта
	float y;  // координата Y объекта
	float speed; // общая скорость перемещения
	float moveTimer; // таймер для перемещения

	// Физические параметры
	int w;    // ширина объекта
	int h;    // высота объекта
	int health; // здоровье объекта

	// Флаги состояний
	bool life;     // живой ли объект
	bool isMove;   // движется ли объект
	bool onGround; // находится ли на земле

	// Графические компоненты
	Texture texture; // текстура объекта
	Sprite sprite;   // спрайт объекта
	String name;     // имя объекта

	// Конструктор класса Entity
	Entity(Image& image, String Name, float X, float Y, int W, int H) {
		x = X; // установка начальной позиции X
		y = Y; // установка начальной позиции Y
		w = W; // установка ширины
		h = H; // установка высоты
		name = Name; // присвоение имени
		moveTimer = 0; // обнуление таймера

		speed = 0;   // начальная скорость
		health = 100; // полное здоровье
		dx = 0; // начальная скорость по X
		dy = 0; // начальная скорость по Y

		life = true;    // объект живой
		onGround = false; // не на земле
		isMove = false;   // не движется

		texture.loadFromImage(image); // загрузка текстуры
		sprite.setTexture(texture);    // установка текстуры спрайту
		sprite.setOrigin(w / 2, h / 2); // центрирование спрайта
	}

	// Метод получения хитбокса объекта
	FloatRect getRect() { // ф-ция получения прямоугольника
		return FloatRect(x, y, w, h); // возврат прямоугольника для коллизий
	}

	// Чисто виртуальный метод обновления (должен быть реализован в потомках)
	virtual void update(float time) = 0;
};


////////////////////////////////////////// КЛАСС ИГРОКА ///////////////////////////////

class Player : public Entity {
private:
	// Звуковые эффекты
	Sound jumpSound;       // Звук прыжка
	Sound deadSound;       // Звук смерти при столкновении

	// Система уровней и позиционирование
	int currentLevel;      // Текущий номер уровня
	Level& level;          // Ссылка на объект уровня для взаимодействия
	float startX;          // Стартовая X-координата игрока
	float startY;          // Стартовая Y-координата игрока

	// Таймеры и состояния
	sf::Clock teleportClock;       // Таймер для задержки телепортации
	bool isOutOfBounds;            // Флаг выхода за границы уровня
	static const float RESURRECTION_DELAY; // Задержка перед возрождением (0.3 сек)
	sf::Clock resurrectionClock;   // Таймер для отсчета времени возрождения
	bool isResurrecting;           // Флаг блокировки управления при возрождении

	// Анимация
	float CurrentFrame;    // Текущий кадр анимации

public:


	// Состояния игрока
	enum { left, right, up, down, jump, stay } state; // Возможные состояния движения
	bool needRestartLevel; // Флаг необходимости перезапуска уровня
	int playerScore;  // Общий счет
	int levelScore;   // Счет текущего уровня       
	bool isVisible;        // Видимость игрока (используется при смерти)
	static int deathCount; // Статический счетчик смертей



	void AddScore(int points) {
		levelScore += points;
	}
	// Обработка смерти игрока
	void die() {
		if (!isOutOfBounds) {
			isVisible = false;
			teleportClock.restart();
			isOutOfBounds = true;
			deadSound.setVolume(soundEffectsVolume); // Обновляем громкость
			deadSound.play();
			isResurrecting = true;
			resurrectionClock.restart();
			deathCount++;

			// Сбрасываем монетки уровня
			collectedCoinsMap[currentLevel].clear();
			levelScore = 0;
		}
	}
	void completeLevel() {
		playerScore += levelScore;
		levelScore = 0;
	}

	// Конструктор игрока
	Player(Image& image, String Name, Level& lev, float X, float Y, int W, int H,
		SoundBuffer& jumpBuffer, SoundBuffer& deadBuffer, int currentLevel, int initialScore = 0)
		: Entity(image, Name, X, Y, W, H), // Инициализация базового класса
		level(lev),               // Инициализация ссылки на уровень
		jumpSound(jumpBuffer),    // Инициализация звука прыжка
		deadSound(deadBuffer),    // Инициализация звука смерти
		startX(X),                // Сохранение стартовой позиции X
		startY(Y),                // Сохранение стартовой позиции Y
		isOutOfBounds(false),     // Инициализация флага выхода за границы
		isResurrecting(true),     // Начальное состояние возрождения
		CurrentFrame(0),          // Начальный кадр анимации
		currentLevel(currentLevel), // Установка текущего уровня
		playerScore(initialScore) // Инициализация счета
	{
		resurrectionClock.restart(); // Запуск таймера возрождения
		needRestartLevel = false;    // Сброс флага перезапуска уровня
		state = stay;                // Начальное состояние - без движения
		obj = lev.GetObjects("solid"); // Получение твердых объектов уровня
		isVisible = true;            // Игрок видим при создании
		jumpSound.setVolume(soundEffectsVolume);
		deadSound.setVolume(soundEffectsVolume);
		// Настройка спрайта для первого игрока
		if (name == "Player1") {
			sprite.setTextureRect(IntRect(0, 0, w, h)); // Установка текстурного прямоугольника
			if (health <= 0) { life = false; } // Проверка жизней
		}
	}
	// Обработка управления
	void control(float time) {  // Метод обработки управления игроком
		sprite.setTextureRect(IntRect(0, 0, 37, 50)); // Установка прямоугольника текстуры для спрайта (базовая анимация стояния)

		if (!isResurrecting) { // Проверка, не находится ли игрок в процессе воскрешения
			// Инвертированное управление для 9 и 10 уровней
			if (currentLevel == 9 || currentLevel == 10) { // Проверка текущего уровня
				if (Keyboard::isKeyPressed(Keyboard::Left) || Keyboard::isKeyPressed(Keyboard::A)) { // Проверка нажатия клавиши влево
					state = right; // Установка состояния движения вправо (инвертированное)
					speed = 0.15;  // Установка скорости движения
					CurrentFrame += 0.005 * time; // Обновление текущего кадра анимации
					if (CurrentFrame > 2) CurrentFrame -= 2; // Циклическое переключение кадров анимации
					sprite.setTextureRect(IntRect(39 * int(CurrentFrame), 104, 37, 50)); // Установка прямоугольника текстуры для анимации движения вправо
				}
				if (Keyboard::isKeyPressed(Keyboard::Right) || Keyboard::isKeyPressed(Keyboard::D)) { // Проверка нажатия клавиши вправо
					state = left; // Установка состояния движения влево (инвертированное)
					speed = 0.15; // Установка скорости движения
					CurrentFrame += 0.005 * time; // Обновление текущего кадра анимации
					if (CurrentFrame > 2) CurrentFrame -= 2; // Циклическое переключение кадров анимации
					sprite.setTextureRect(IntRect(39 * int(CurrentFrame), 52, 37, 50)); // Установка прямоугольника текстуры для анимации движения влево
				}
			}
			else { // Стандартное управление для остальных уровней
				if (Keyboard::isKeyPressed(Keyboard::Left) || Keyboard::isKeyPressed(Keyboard::A)) { // Проверка нажатия клавиши влево
					state = left; // Установка состояния движения влево
					speed = 0.15; // Установка скорости движения
					CurrentFrame += 0.005 * time; // Обновление текущего кадра анимации
					if (CurrentFrame > 2) CurrentFrame -= 2; // Циклическое переключение кадров анимации
					sprite.setTextureRect(IntRect(39 * int(CurrentFrame), 52, 37, 50)); // Установка прямоугольника текстуры для анимации движения влево
				}
				if (Keyboard::isKeyPressed(Keyboard::Right) || Keyboard::isKeyPressed(Keyboard::D)) { // Проверка нажатия клавиши вправо
					state = right; // Установка состояния движения вправо
					speed = 0.15; // Установка скорости движения
					CurrentFrame += 0.005 * time; // Обновление текущего кадра анимации
					if (CurrentFrame > 2) CurrentFrame -= 2; // Циклическое переключение кадров анимации
					sprite.setTextureRect(IntRect(39 * int(CurrentFrame), 104, 37, 50)); // Установка прямоугольника текстуры для анимации движения вправо
				}
			}
		}

		// Обработка прыжка
		if ((Keyboard::isKeyPressed(Keyboard::Up)) && (onGround) || Keyboard::isKeyPressed(Keyboard::W) && (onGround)) { // Проверка нажатия клавиши прыжка и нахождения на земле
			state = jump; // Установка состояния прыжка
			dy = -0.6;      // Установка вертикальной скорости (импульс прыжка)
			onGround = false; // Сброс флага нахождения на земле
			jumpSound.setVolume(soundEffectsVolume); // Установка громкости звука прыжка
			jumpSound.play(); // Воспроизведение звука прыжка
		}
	}

	// Проверка столкновений с картой
	void checkCollisionWithMap(float Dx, float Dy) { // Метод проверки столкновений с объектами карты
		for (int i = 0; i < obj.size(); i++) { // Цикл по всем объектам карты
			if (getRect().intersects(obj[i].rect)) { // Проверка пересечения с текущим объектом
				// Обработка разных типов объектов
				if (obj[i].name == "solid" || obj[i].name == "hard") { // Проверка типа объекта (твердый)
					if (Dy > 0) { // Столкновение снизу
						y = obj[i].rect.top - h; // Коррекция позиции по Y
						dy = 0;     // Сброс вертикальной скорости
						onGround = true; // Установка флага нахождения на земле
					}
					if (Dy < 0) { // Столкновение сверху
						y = obj[i].rect.top + obj[i].rect.height; // Коррекция позиции по Y
						dy = 0; // Сброс вертикальной скорости
					}
					if (Dx > 0) { // Столкновение справа
						x = obj[i].rect.left - w; // Коррекция позиции по X
					}
					if (Dx < 0) { // Столкновение слева
						x = obj[i].rect.left + obj[i].rect.width; // Коррекция позиции по X
					}
				}
			}
		}
	}

	// Обновление состояния игрока
	void update(float time) { // Метод обновления состояния игрока
		control(time); // Вызов метода обработки управления

		onGround = false; // Сброс флага нахождения на земле в каждом кадре

		// Установка скорости движения в зависимости от состояния
		switch (state) { // Проверка текущего состояния игрока
		case right: dx = speed; break;  // Движение вправо
		case left: dx = -speed; break;  // Движение влево
		case jump: break;               // Прыжок (без изменения горизонтальной скорости)
		case stay: break;               // Без движения
		}

		x += dx * time; // Обновление позиции по X с учетом скорости
		checkCollisionWithMap(dx, 0); // Проверка коллизий по X

		y += dy * time; // Обновление позиции по Y с учетом скорости
		checkCollisionWithMap(0, dy); // Проверка коллизий по Y

		// Проверка выхода за границы уровня
		sf::Vector2f levelSize = level.GetLevelSize(); // Получение размеров уровня
		bool currentOutOfBounds = (x < 0 || x + w > levelSize.x || y < 0 || y + h > levelSize.y); // Проверка выхода за границы
		if (currentOutOfBounds || isOutOfBounds) { // Если игрок вышел за границы
			if (!isOutOfBounds) die(); // Вызов метода смерти при первом выходе
			if (teleportClock.getElapsedTime().asSeconds() >= 0.3f) { // Проверка таймера телепортации
				needRestartLevel = true; // Установка флага необходимости перезапуска уровня
				isOutOfBounds = false; // Сброс флага выхода за границы
			}
		}
		else {
			isOutOfBounds = false; // Сброс флага выхода за границы
		}

		// Обработка процесса возрождения
		if (isResurrecting) { // Проверка состояния воскрешения
			if (resurrectionClock.getElapsedTime().asSeconds() >= RESURRECTION_DELAY) { // Проверка таймера воскрешения
				isResurrecting = false; // Сброс флага воскрешения
				isVisible = true; // Восстановление видимости игрока
			}
		}

		sprite.setPosition(x + w / 2, y + h / 2); // Обновление позиции спрайта (центрирование)

		if (health <= 0) { life = false; } // Проверка здоровья (смерть при нулевом здоровье)

		if (!isMove) { speed = 0; } // Сброс скорости при отсутствии движения

		dy = dy + 0.0015 * time; // Применение гравитации (увеличение вертикальной скорости)
	}
	};

const float Player::RESURRECTION_DELAY = 0.3f; // Время задержки перед воскрешением



// Объявление функции для отображения загрузочного экрана
void showLoadingScreen(RenderWindow& window) {
	// Установка ограничения частоты кадров окна
	window.setFramerateLimit(100);
	// Объявление текстуры для фона
	Texture bgTexture;
	// Загрузка фоновой текстуры из файла
	bgTexture.loadFromFile("images/Zaryzka.png");

	// Флаг для запроса пропуска анимации (false - не пропущено)
	bool skipRequested = false; // Флаг для пропуска анимации

	// Создание спрайта фона из текстуры
	Sprite background(bgTexture);
	// Установка масштаба фона пропорционально размеру окна
	background.setScale(
		// Расчет масштаба по ширине
		static_cast<float>(window.getSize().x) / bgTexture.getSize().x,
		// Расчет масштаба по высоте
		static_cast<float>(window.getSize().y) / bgTexture.getSize().y
	);

	// Объявление текстуры для полосы загрузки
	Texture barTexture;
	// Загрузка текстуры полосы из файла
	barTexture.loadFromFile("images/polosa.png");

	// Создание спрайта полосы загрузки
	Sprite progressBar(barTexture);

	// Объявление буфера для звука букв
	sf::SoundBuffer letterSoundBuffer;
	// Загрузка звука из файла
	letterSoundBuffer.loadFromFile("music/Bykva.wav");

	// Объявление звукового объекта
	sf::Sound letterSound;
	// Привязка буфера звука к объекту
	letterSound.setBuffer(letterSoundBuffer);

	// Ширина спрайта полосы загрузки
	const float barWidth = progressBar.getLocalBounds().width;
	// Ширина окна приложения
	const float windowWidth = window.getSize().x;
	// Высота окна приложения
	const float windowHeight = window.getSize().y;
	// Y-координата для позиционирования полосы загрузки
	const float barY = (windowHeight - barTexture.getSize().y) / 1.37;

	// Установка начальной позиции полосы (левее видимой области)
	progressBar.setPosition(-barWidth, barY);

	// Объявление текстуры персонажа
	Texture characterTexture;
	// Загрузка текстуры персонажа из файла
	characterTexture.loadFromFile("images/zagryzakapers.png");

	// Создание спрайта персонажа
	Sprite character(characterTexture);

	// Параметры анимации персонажа
	// Количество кадров в анимации
	const int characterFrames = 2;      // Количество кадров анимации
	// Ширина одного кадра анимации
	const float frameWidth = 90.0f;     // Ширина одного кадра
	// Высота одного кадра анимации
	const float frameHeight = 122.0f;   // ВЫСОТА КАДРА ИСПРАВЛЕНА (было 61)
	// Текущий отображаемый кадр
	float currentFrame = 0.0f;          // Текущий кадр
	// Скорость анимации (секунды на кадр)
	const float animationSpeed = 0.1f;  // Скорость смены кадров (сек/кадр)

	// Стартовая позиция персонажа (левее экрана)
	const float characterStartX = -frameWidth;
	// Y-координата персонажа (над полосой загрузки)
	const float characterY = barY - frameHeight; // Над полосой загрузки
	// Текущая X-координата персонажа
	float characterX = characterStartX;

	// Настройка прямоугольника текстуры для анимации
	character.setTextureRect(IntRect(0, 0, frameWidth, frameHeight));
	// Установка позиции персонажа
	character.setPosition(characterX, characterY);

	// Подготовка к отображению слова "Валли"
	// Вектор текстур для букв (4 уникальные буквы)
	std::vector<sf::Texture> letterTextures(4); // Явно указываем пространство имён sf
	// Вектор спрайтов для букв
	std::vector<sf::Sprite> letters;
	// Флаги видимости букв (5 букв)
	std::vector<bool> letterVisible(5, false);
	// Время появления каждой буквы (в секундах)
	std::vector<float> letterTimes = { 2.0f, 2.7f, 3.4f, 4.1f, 4.8f };

	// Загрузка текстур для каждой буквы
	letterTextures[0].loadFromFile("images/В.png");
	letterTextures[1].loadFromFile("images/А.png");
	letterTextures[2].loadFromFile("images/Л.png");
	letterTextures[3].loadFromFile("images/И.png");
	// Создание спрайтов для букв
	for (int i = 0; i < 5; i++) {
		// Создание спрайта для текущей буквы
		Sprite letter;
		// Привязка соответствующей текстуры для буквы
		// Буква "В"
		if (i == 0) letter.setTexture(letterTextures[0]);      // В
		// Буква "А"
		else if (i == 1) letter.setTexture(letterTextures[1]); // А
		// Буквы "Л" (дважды)
		else if (i == 2 || i == 3) letter.setTexture(letterTextures[2]); // Л (дважды)
		// Буква "И"
		else letter.setTexture(letterTextures[3]); // И

		// Добавление спрайта в вектор
		letters.push_back(letter);
	}

	// Расчет общей ширины слова
	float totalWidth = 0;
	// Суммирование ширины всех букв
	for (int i = 0; i < 5; i++) {
		totalWidth += letters[i].getLocalBounds().width;
	}
	// Расстояние между буквами
	const float spacing = 10.0f;
	// Добавление промежутков между буквами
	totalWidth += spacing * 4;

	// Стартовая позиция для центрирования слова
	float startX = (windowWidth - totalWidth) / 2;
	// Текущая позиция при расстановке букв
	float currentX = startX;
	// Y-координата для букв (20% высоты окна)
	const float letterY = windowHeight * 0.2f; // 20% от верха экрана

	// Растановка позиций для каждой буквы
	for (int i = 0; i < 5; i++) {
		// Установка позиции текущей буквы
		letters[i].setPosition(currentX, letterY);
		// Смещение позиции для следующей буквы
		currentX += letters[i].getLocalBounds().width + spacing;
	}

	// Таймер для общего времени анимации
	Clock clock;
	// Таймер для анимации персонажа
	Clock animationClock; // Отдельные часы для анимации
	// Время движения полосы загрузки (секунды)
	const float barDuration = 6.0f;     // ВЫСТАВЛЕНО ОДИНАКОВОЕ ВРЕМЯ С ПЕРСОНАЖЕМ
	// Время движения персонажа (секунды)
	const float characterDuration = 6.6f; // Время движения персонажа
	// Дистанция для перемещения полосы
	const float barTotalDistance = barWidth; // ПРАВИЛЬНАЯ ДИСТАНЦИЯ ДЛЯ ПОЛОСЫ

	// Главный цикл отрисовки
	while (window.isOpen()) {
		// Обработка событий
		Event event;
		// Проверка всех событий в очереди
		while (window.pollEvent(event)) {
			// Закрытие окна
			if (event.type == Event::Closed)
				window.close();
			// Обработка нажатия клавиши
			if (event.type == Event::KeyPressed) {
				// Установка флага пропуска анимации
				skipRequested = true;
			}

		}

		// Получение времени с начала анимации
		float elapsed = clock.getElapsedTime().asSeconds();

		// Расчет прогресса полосы загрузки (0.0 - 1.0)
		float barProgress = elapsed / barDuration;
		// Ограничение прогресса максимум 100%
		if (barProgress > 1.0f) barProgress = 1.0f;
		// Расчет смещения полосы
		float barOffset = barProgress * barTotalDistance;
		// Расчет новой позиции полосы
		float newBarX = -barWidth + barOffset;
		// Фиксация полосы у правого края
		if (newBarX > 0) newBarX = 0;
		// Обновление позиции полосы
		progressBar.setPosition(newBarX, barY);

		// Расчет прогресса движения персонажа
		float characterProgress = elapsed / characterDuration;
		// Ограничение прогресса максимум 100%
		if (characterProgress > 1.0f) characterProgress = 1.0f;
		// Расчет новой позиции персонажа
		characterX = characterStartX + characterProgress * (windowWidth + frameWidth * 2);
		// Обновление позиции персонажа
		character.setPosition(characterX, characterY);

		// Обновление анимации персонажа
		// Проверка необходимости смены кадра
		if (animationClock.getElapsedTime().asSeconds() > animationSpeed) {
			// Переход к следующему кадру
			currentFrame += 1;
			// Зацикливание анимации
			if (currentFrame >= characterFrames) currentFrame = 0;

			// Установка прямоугольника текстуры для текущего кадра
			character.setTextureRect(IntRect(
				// X-координата кадра в текстуре
				static_cast<int>(currentFrame) * frameWidth,
				// Y-координата кадра в текстуре
				0,
				// Ширина выводимой области
				frameWidth,
				// Высота выводимой области
				frameHeight
			));
			// Сброс таймера анимации
			animationClock.restart();
		}

		// Активация букв по расписанию
		for (int i = 0; i < 5; i++) {
			// Проверка времени появления и статуса видимости
			if (!letterVisible[i] && elapsed >= letterTimes[i]) {
				// Показ текущей буквы
				letterVisible[i] = true;

				// Воспроизведение звука появления буквы
				// Проверка, не играет ли уже звук
				if (letterSound.getStatus() != sf::Sound::Playing) {
					// Проигрывание звука
					letterSound.play(); // Проигрываем, если звук не занят
				}
				else {
					// Создание временного звукового объекта
					sf::Sound tempSound;
					// Привязка звукового буфера
					tempSound.setBuffer(letterSoundBuffer);
					// Проигрывание звука
					tempSound.play();
				}
			}
		}

		// Отрисовка кадра
		// Очистка окна
		window.clear();
		// Отрисовка фона
		window.draw(background);
		// Отрисовка полосы загрузки
		window.draw(progressBar);
		// Отрисовка персонажа поверх полосы
		window.draw(character); // Рисуем персонажа поверх полосы

		// Отрисовка видимых букв слова
		for (int i = 0; i < 5; i++) {
			// Проверка видимости текущей буквы
			if (letterVisible[i]) {
				// Отрисовка буквы
				window.draw(letters[i]);
			}
		}

		// Отображение нарисованного кадра
		window.display();

		// Условие завершения анимации
		// По истечении времени или запросу пропуска
		if (elapsed >= characterDuration || skipRequested) {
			// Выход из цикла загрузочного экрана
			break;
		}
	}
}

////////////////////////////////////////// КЛАСС СЛАЙДЕРА ///////////////////////////////
// Класс для создания и управления слайдером
class Slider {
private:
	sf::RectangleShape track;    // Прямоугольник - дорожка слайдера
	sf::CircleShape thumb;       // Круг - ползунок слайдера
	float minValue;              // Минимальное значение слайдера
	float maxValue;              // Максимальное значение слайдера
	float* value;                // Указатель на переменную, хранящую текущее значение
	bool isDragging;             // Флаг: перетаскивается ли ползунок в данный момент
	sf::Vector2f position;       // Позиция левого края слайдера (X, Y)
	float width;                 // Ширина дорожки слайдера

public:
	// Конструктор слайдера: инициализация параметров
	Slider(float x, float y, float w, float min, float max, float* val)
		: minValue(min), maxValue(max), value(val), isDragging(false), // Инициализация членов класса
		position(x, y), width(w) { // Инициализация позиции и ширины

		// Настройка внешнего вида дорожки
		track.setSize(sf::Vector2f(w, 10)); // Установка размеров дорожки
		track.setPosition(x, y + 10); // Позиционирование дорожки (смещение по Y для центрирования)
		track.setFillColor(sf::Color(100, 100, 100)); // Цвет заполнения дорожки
		track.setOutlineThickness(2); // Толщина контура
		track.setOutlineColor(sf::Color(150, 150, 150)); // Цвет контура

		// Настройка внешнего вида ползунка
		thumb.setRadius(15); // Радиус ползунка
		thumb.setFillColor(sf::Color::Magenta); // Цвет заполнения
		thumb.setOutlineThickness(2); // Толщина контура
		thumb.setOutlineColor(sf::Color(200, 200, 200)); // Цвет контура

		// Вызов метода для первоначального позиционирования ползунка
		updateThumbPosition();
	}

	// Метод для изменения позиции слайдера
	void setPosition(float x, float y) {
		position.x = x; // Обновление X-координаты
		position.y = y; // Обновление Y-координаты
		track.setPosition(x, y + 10); // Перемещение дорожки
		updateThumbPosition(); // Обновление позиции ползунка
	}

	// Метод для изменения ширины слайдера
	void setWidth(float w) {
		width = w; // Обновление ширины
		track.setSize(sf::Vector2f(w, 10)); // Изменение размера дорожки
		updateThumbPosition(); // Обновление позиции ползунка
	}

	// Обновление позиции ползунка на основе текущего значения
	void updateThumbPosition() {
		// Нормализация значения в диапазон [0, 1]
		float normalizedValue = (*value - minValue) / (maxValue - minValue);
		// Расчет X-координаты центра ползунка
		float thumbX = position.x + normalizedValue * width - thumb.getRadius();
		// Установка позиции ползунка
		thumb.setPosition(thumbX, position.y);
	}

	// Обработка событий мыши для слайдера
	void handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
		// Обработка нажатия кнопки мыши
		if (event.type == sf::Event::MouseButtonPressed) {
			if (event.mouseButton.button == sf::Mouse::Left) {
				// Преобразование координат курсора
				sf::Vector2f mousePos = window.mapPixelToCoords(
					sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

				// Проверка клика на ползунке
				if (thumb.getGlobalBounds().contains(mousePos)) {
					isDragging = true; // Начало перетаскивания
				}
				// Проверка клика на дорожке
				else if (track.getGlobalBounds().contains(mousePos)) {
					// Расчет нового значения по позиции клика
					float relativeX = mousePos.x - position.x;
					*value = minValue + (relativeX / width) * (maxValue - minValue);
					// Обновление позиции ползунка
					updateThumbPosition();
				}
			}
		}
		// Обработка отпускания кнопки мыши
		else if (event.type == sf::Event::MouseButtonReleased) {
			if (event.mouseButton.button == sf::Mouse::Left) {
				isDragging = false; // Конец перетаскивания
			}
		}
		// Обработка движения мыши при перетаскивании
		else if (event.type == sf::Event::MouseMoved && isDragging) {
			// Преобразование координат курсора
			sf::Vector2f mousePos = window.mapPixelToCoords(
				sf::Vector2i(event.mouseMove.x, event.mouseMove.y));

			// Ограничение X-координаты в пределах дорожки
			float newX = std::max(position.x, std::min(mousePos.x, position.x + width));
			// Расчет нового значения
			float normalizedValue = (newX - position.x) / width;
			*value = minValue + normalizedValue * (maxValue - minValue);

			// Обновление позиции ползунка
			updateThumbPosition();
		}
	}

	// Отрисовка слайдера в окне
	void draw(sf::RenderWindow& window) {
		window.draw(track); // Отрисовка дорожки
		window.draw(thumb); // Отрисовка ползунка
	}
};

// Глобальная переменная для громкости
float globalVolume = 50.0f; // Громкость музыки (значение по умолчанию)

// Функция сохранения игры
void saveGame(int level, int score, int deaths,
	const std::map<int, std::set<std::pair<float, float>>>& coins, // Коллекция собранных монет
	const std::string& skin, // Текущий скин
	bool item1Purchased, bool item3Purchased, // Статус покупок
	float volume) { // Громкость (аргумент)
	std::ofstream file("savegame.dat", std::ios::binary); // Открытие файла для записи
	if (file.is_open()) {
		// Запись основных параметров игры
		file.write(reinterpret_cast<const char*>(&level), sizeof(level));
		file.write(reinterpret_cast<const char*>(&score), sizeof(score));
		file.write(reinterpret_cast<const char*>(&deaths), sizeof(deaths));

		// Запись данных о монетах
		size_t mapSize = coins.size(); // Количество уровней с монетами
		file.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));
		for (const auto& entry : coins) {
			int lvl = entry.first; // Номер уровня
			file.write(reinterpret_cast<const char*>(&lvl), sizeof(lvl));

			size_t setSize = entry.second.size(); // Количество монет на уровне
			file.write(reinterpret_cast<const char*>(&setSize), sizeof(setSize));
			for (const auto& coord : entry.second) {
				// Запись координат монеты
				file.write(reinterpret_cast<const char*>(&coord.first), sizeof(coord.first));
				file.write(reinterpret_cast<const char*>(&coord.second), sizeof(coord.second));
			}
		}

		// Запись данных о скине
		size_t skinLength = skin.size(); // Длина строки скина
		file.write(reinterpret_cast<const char*>(&skinLength), sizeof(skinLength));
		file.write(skin.c_str(), skinLength); // Запись строки

		// Запись статуса покупок
		file.write(reinterpret_cast<const char*>(&item1Purchased), sizeof(item1Purchased));
		file.write(reinterpret_cast<const char*>(&item3Purchased), sizeof(item3Purchased));

		// Сохранение громкости в файл
		file.write(reinterpret_cast<const char*>(&globalVolume), sizeof(globalVolume));
		file.write(reinterpret_cast<const char*>(&soundEffectsVolume), sizeof(soundEffectsVolume));
		file.close(); // Закрытие файла
	}
}

// Функция загрузки игры
bool loadGame(int& level, int& score, int& deaths,
	std::map<int, std::set<std::pair<float, float>>>& coins, // Коллекция для монет
	std::string& skin, // Строка для скина
	bool& item1Purchased, bool& item3Purchased, // Переменные для статуса покупок
	float& volume) { // Ссылка для громкости (не используется)
	std::ifstream file("savegame.dat", std::ios::binary); // Открытие файла для чтения
	if (!file.is_open()) return false; // Проверка успешности открытия

	// Чтение основных параметров
	file.read(reinterpret_cast<char*>(&level), sizeof(level));
	file.read(reinterpret_cast<char*>(&score), sizeof(score));
	file.read(reinterpret_cast<char*>(&deaths), sizeof(deaths));

	// Чтение данных о монетах
	size_t mapSize;
	file.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));
	for (size_t i = 0; i < mapSize; ++i) {
		int lvl; // Номер уровня
		file.read(reinterpret_cast<char*>(&lvl), sizeof(lvl));

		size_t setSize; // Количество монет
		file.read(reinterpret_cast<char*>(&setSize), sizeof(setSize));
		std::set<std::pair<float, float>> coinSet; // Множество координат
		for (size_t j = 0; j < setSize; ++j) {
			float x, y; // Координаты монеты
			file.read(reinterpret_cast<char*>(&x), sizeof(x));
			file.read(reinterpret_cast<char*>(&y), sizeof(y));
			coinSet.insert({ x, y }); // Добавление в множество
		}
		coins[lvl] = coinSet; // Сохранение для уровня
	}

	// Чтение данных о скине
	size_t skinLength;
	file.read(reinterpret_cast<char*>(&skinLength), sizeof(skinLength));
	char* skinBuffer = new char[skinLength + 1]; // Буфер для строки
	file.read(skinBuffer, skinLength); // Чтение строки
	skinBuffer[skinLength] = '\0'; // Добавление терминатора
	skin = std::string(skinBuffer); // Создание строки
	delete[] skinBuffer; // Освобождение памяти

	// Чтение статуса покупок
	file.read(reinterpret_cast<char*>(&item1Purchased), sizeof(item1Purchased));
	file.read(reinterpret_cast<char*>(&item3Purchased), sizeof(item3Purchased));

	// Загрузка громкости (если данные доступны)
	if (!file.eof()) {
		file.read(reinterpret_cast<char*>(&globalVolume), sizeof(globalVolume));
		file.read(reinterpret_cast<char*>(&soundEffectsVolume), sizeof(soundEffectsVolume));
	}
	else {
		soundEffectsVolume = 50.0f; // Значение по умолчанию при отсутствии данных
	}
	return true; // Успешная загрузка
}

// Объявление функции меню
int menu(RenderWindow& window, bool hasSave, int& currentScore, int& savedLevel,
	std::map<int, std::set<std::pair<float, float>>>& collectedCoinsMap,
	std::string& currentSkin,
	bool& item1Purchased, bool& item3Purchased) {

	// Объявление буфера для звука наведения
	SoundBuffer hoverBuffer;
	// Объявление звукового объекта
	Sound hoverSound;
	// Попытка загрузить звуковой файл
	if (!hoverBuffer.loadFromFile("music/button.ogg")) {
		// Вывод ошибки при неудачной загрузке
		std::cerr << "Failed to load button sound!" << std::endl;
	}
	else {
		// Установка буфера для звука
		hoverSound.setBuffer(hoverBuffer);
		// Установка громкости звука
		hoverSound.setVolume(30.f); // Установка комфортной громкости
	}

	// Объявление шрифта для текста
	Font font;
	// Попытка загрузки шрифта из файла
	if (!font.loadFromFile("Text.ttf")) {
		// Вывод ошибки при неудачной загрузке
		std::cerr << "Failed to load font!" << std::endl;
		// Возврат ошибки
		return -1;
	}

	// Создание текстового объекта для отображения очков
	Text pointsText;
	// Установка шрифта для текста
	pointsText.setFont(font);
	// Установка размера символов
	pointsText.setCharacterSize(50);
	// Установка цвета текста
	pointsText.setFillColor(sf::Color::White);
	// Установка строки с текущими очками
	pointsText.setString("POINTS: " + std::to_string(currentScore));
	// Позиционирование текста
	pointsText.setPosition(1550, 20);
	// Установка ограничения частоты кадров
	window.setFramerateLimit(100);
	// Сохранение начального вида окна
	View initialView = window.getView();

	// Объявление текстур для элементов меню
	Texture menuTexture1, menuTexture1_h, menuTexture2, menuTexture2_h,
		menuTexture4, menuTexture4_h, menuTexture3, menuTexture3_h,
		menuTexture5, menuTexture5_h, aboutTexture, aboutTextureGame, menuBackground,
		menuTexture6, menuTexture6_h, shopBackgroundTexture, installTexture, byTexture,
		menuTexture7, menuTexture7_h, settingsMenuTexture, menuTexture8, menuTexture8_h;
	// Загрузка текстуры для кнопки справки
	menuTexture8.loadFromFile("images/spravka.png");
	// Загрузка текстуры для кнопки справки (состояние наведения)
	menuTexture8_h.loadFromFile("images/spravkah.png");

	// Загрузка основных текстур меню
	menuTexture1.loadFromFile("images/StartGame.png");
	menuTexture2.loadFromFile("images/Avtor.png");
	menuTexture4.loadFromFile("images/Ob Game.png");
	menuTexture3.loadFromFile("images/Exit.png");
	menuTexture5.loadFromFile("images/Continue.png");
	menuTexture6.loadFromFile("images/shop.png");
	menuTexture7.loadFromFile("images/settings.png");

	// Загрузка текстур для состояния наведения
	menuTexture1_h.loadFromFile("images/StartGameh.png");
	menuTexture2_h.loadFromFile("images/Avtorh.png");
	menuTexture4_h.loadFromFile("images/Ob Gameh.png");
	menuTexture3_h.loadFromFile("images/Exith.png");
	menuTexture5_h.loadFromFile("images/Continueh.png");
	menuTexture6_h.loadFromFile("images/shoph.png");
	menuTexture7_h.loadFromFile("images/settingsh.png");

	// Загрузка вспомогательных текстур
	aboutTexture.loadFromFile("images/about.png");
	aboutTextureGame.loadFromFile("images/aboutGame.png");
	menuBackground.loadFromFile("images/menu.png");
	shopBackgroundTexture.loadFromFile("images/skins.png");
	installTexture.loadFromFile("images/installed.png");
	byTexture.loadFromFile("images/by.png");
	settingsMenuTexture.loadFromFile("images/settingsmenu.png");

	// Создание спрайтов для кнопок
	Sprite menu1(menuTexture1), menu2(menuTexture2), menu4(menuTexture4),
		menu3(menuTexture3), menu5(menuTexture5), menuBg(menuBackground),
		menu6(menuTexture6), installBtn(installTexture), byBtn(byTexture),
		menu7(menuTexture7), menu8(menuTexture8);

	// Получение текущего размера окна
	Vector2u windowSize = window.getSize();
	// Флаги для отображения подменю
	bool showAbout = false, showAboutGame = false, showShop = false, showSettings = false;

	// Лямбда-функция для обновления позиций кнопок
	auto updateButtonPositions = [&]() {
		// Обновление размера окна
		windowSize = window.getSize();
		// Расчет коэффициента масштабирования
		float scale = windowSize.y / 768.0f;

		// Установка масштаба для кнопок
		menu1.setScale(scale, scale);
		menu2.setScale(scale, scale);
		menu4.setScale(scale, scale);
		menu3.setScale(scale, scale);
		menu5.setScale(scale, scale);
		menu6.setScale(scale, scale);
		menu7.setScale(scale, scale);

		// Создание списка кнопок
		std::vector<Sprite*> buttons;
		// Добавление кнопки продолжения при наличии сохранения
		if (hasSave) buttons.push_back(&menu5);
		// Добавление остальных кнопок
		buttons.push_back(&menu1);
		buttons.push_back(&menu2);
		buttons.push_back(&menu4);
		buttons.push_back(&menu6);
		buttons.push_back(&menu7);
		buttons.push_back(&menu3);
		buttons.push_back(&menu8);
		// Расчет общей высоты кнопок
		float totalHeight = 0;
		for (auto& btn : buttons) {
			totalHeight += btn->getGlobalBounds().height - 1 * scale;
		}
		totalHeight -= 70 * scale;

		// Расчет стартовой позиции по Y
		float startY = (windowSize.y - totalHeight) / 2;
		float yPos = startY;
		// Позиционирование кнопок
		for (auto& btn : buttons) {
			btn->setPosition(
				(windowSize.x - btn->getGlobalBounds().width) / 2,
				yPos
			);
			yPos += btn->getGlobalBounds().height + 10 * scale;
		}

		// Позиционирование кнопки справки
		float settingsY = menu7.getPosition().y;
		float helpX = windowSize.x - menu8.getGlobalBounds().width - 50 * scale; // Справа с отступом
		menu8.setPosition(helpX, settingsY); // На том же уровне, что и кнопка настроек
		};

	// Первоначальное позиционирование кнопок
	updateButtonPositions();

	// Флаг работы меню
	bool isMenu = true;
	// Номер активного пункта меню
	int menuNum = 0;
	// Указатель на предыдущую кнопку под курсором
	Sprite* prevHoveredButton = nullptr; // Трекер предыдущей кнопки
	// Флаг для сброса состояния после подменю
	bool wasInSubMenu = false; // Для сброса состояния после подменю

	// Главный цикл меню
	while (isMenu) {
		Event event;
		// Обработка событий в очереди
		while (window.pollEvent(event)) {
			// Обработка изменения размера окна
			if (event.type == Event::Resized) {
				// Обновление области просмотра
				FloatRect visibleArea(0, 0, event.size.width, event.size.height);
				window.setView(View(visibleArea));
				// Обновление позиций кнопок
				updateButtonPositions();
			}
			// Обработка закрытия окна
			if (event.type == Event::Closed) {
				window.close();
				return 0;
			}
		}

		// Обработка дополнительных экранов
		if (showAbout || showAboutGame || showShop || showSettings) {
			// Выход из подменю по ESC
			if (Keyboard::isKeyPressed(Keyboard::Escape)) {
				showAbout = showAboutGame = showShop = showSettings = false;
			}

			// Обработка экрана магазина
			if (showShop) {
				// Создание фона магазина
				Sprite shopBg(shopBackgroundTexture);
				// Расчет масштаба по X
				float scaleX = static_cast<float>(windowSize.x) / shopBackgroundTexture.getSize().x;
				// Расчет масштаба по Y
				float scaleY = static_cast<float>(windowSize.y) / shopBackgroundTexture.getSize().y;
				// Установка масштаба
				shopBg.setScale(scaleX, scaleY);

				// Обновление текста с очками
				pointsText.setString("POINTS: " + std::to_string(currentScore));
				// Позиционирование текста
				pointsText.setPosition(1550 * scaleX, 20 * scaleY);

				// Загрузка текстур для кнопок магазина
				Texture byTexture, installTexture, installedTexture;
				byTexture.loadFromFile("images/by.png");
				installTexture.loadFromFile("images/install.png");
				installedTexture.loadFromFile("images/installed.png");

				// Проверка установленных скинов
				bool item1Installed = (currentSkin == "pers2.png");
				bool item2Installed = (currentSkin == "pers.png");
				bool item3Installed = (currentSkin == "pers3.png");
				// Предположение, что второй предмет куплен по умолчанию
				bool item2Purchased = true;

				// Создание кнопок для предметов
				Sprite item1Btn, item2Btn, item3Btn;
				// Масштабирование кнопок
				item1Btn.setScale(scaleX, scaleY);
				item2Btn.setScale(scaleX, scaleY);
				item3Btn.setScale(scaleX, scaleY);

				// Позиционирование кнопок
				item1Btn.setPosition(167 * scaleX, 503 * scaleY);
				item2Btn.setPosition(468 * scaleX, 503 * scaleY);
				item3Btn.setPosition(763 * scaleX, 503 * scaleY);

				// Установка текстур в зависимости от состояния
				item1Btn.setTexture(item1Purchased ?
					(item1Installed ? installedTexture : installTexture) : byTexture);
				item2Btn.setTexture(item2Installed ? installedTexture : installTexture);
				item3Btn.setTexture(item3Purchased ?
					(item3Installed ? installedTexture : installTexture) : byTexture);

				// Переменные для обработки кликов
				static bool prevMouseLeftPressed = false;
				bool currentMouseLeftPressed = Mouse::isButtonPressed(Mouse::Left);
				Vector2i mousePos = Mouse::getPosition(window);

				// Обработка кликов по кнопкам
				if (currentMouseLeftPressed && !prevMouseLeftPressed) {
					// Клик по первому предмету
					if (item1Btn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						// Покупка предмета
						if (!item1Purchased && currentScore >= 150) {
							item1Purchased = true;
							currentScore -= 150;
						}
						// Установка скина
						else if (item1Purchased && !item1Installed) {
							currentSkin = "pers2.png";
						}
					}
					// Клик по второму предмету
					else if (item2Btn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						if (item2Purchased && !item2Installed) {
							currentSkin = "pers.png";
						}
					}
					// Клик по третьему предмету
					else if (item3Btn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						if (!item3Purchased && currentScore >= 170) {
							item3Purchased = true;
							currentScore -= 170;
						}
						else if (item3Purchased && !item3Installed) {
							currentSkin = "pers3.png";
						}
					}
					// Сохранение состояния игры
					saveGame(savedLevel, savedScore, Player::deathCount,
						collectedCoinsMap, currentSkin, item1Purchased, item3Purchased, globalVolume);
				}
				prevMouseLeftPressed = currentMouseLeftPressed;

				// Отрисовка магазина
				window.clear();
				window.draw(shopBg);
				window.draw(pointsText);
				window.draw(item1Btn);
				window.draw(item2Btn);
				window.draw(item3Btn);
				window.display();
				continue;
			}
			// Обработка экрана "Об авторе"
			else if (showAbout) {
				// Создание спрайта
				Sprite aboutSprite(aboutTexture);
				// Масштабирование под окно
				aboutSprite.setScale(
					static_cast<float>(windowSize.x) / aboutTexture.getSize().x,
					static_cast<float>(windowSize.y) / aboutTexture.getSize().y
				);
				// Отрисовка
				window.clear();
				window.draw(aboutSprite);
				window.display();
				continue;
			}
			// Обработка экрана "Об игре"
			else if (showAboutGame) {
				// Создание спрайта
				Sprite aboutGameSprite(aboutTextureGame);
				// Масштабирование под окно
				aboutGameSprite.setScale(
					static_cast<float>(windowSize.x) / aboutTextureGame.getSize().x,
					static_cast<float>(windowSize.y) / aboutTextureGame.getSize().y
				);
				// Отрисовка
				window.clear();
				window.draw(aboutGameSprite);
				window.display();
				continue;
			}
			// Обработка экрана настроек
			if (showSettings) {
				// Создание фона настроек
				Sprite settingsMenuSprite(settingsMenuTexture);
				// Расчет масштаба по X
				float scaleX = static_cast<float>(windowSize.x) / settingsMenuTexture.getSize().x;
				// Расчет масштаба по Y
				float scaleY = static_cast<float>(windowSize.y) / settingsMenuTexture.getSize().y;
				// Установка масштаба
				settingsMenuSprite.setScale(scaleX, scaleY);

				// Позиционирование слайдера громкости
				float sliderX = 590 * scaleX;
				float sliderY = 459 * scaleY;
				float sliderWidth = 300 * scaleX;

				// Создание слайдера громкости
				Slider volumeSlider(sliderX, sliderY, sliderWidth, 0, 100, &globalVolume);
				volumeSlider.updateThumbPosition();

				// Настройка текста для громкости
				Text volumeText("", font, 30);
				volumeText.setPosition(350 * scaleX, 390 * scaleY);
				volumeText.setFillColor(sf::Color::White);

				// Текст значения громкости
				Text volumeValueText;
				volumeValueText.setFont(font);
				volumeValueText.setCharacterSize(30);
				volumeValueText.setFillColor(sf::Color::White);
				volumeValueText.setPosition(910 * scaleX, 455 * scaleY);
				volumeValueText.setString(std::to_string(static_cast<int>(globalVolume)) + "%");

				// Создание слайдера для звуковых эффектов
				float soundSliderX = 590 * scaleX;
				float soundSliderY = 559 * scaleY; // Позиция ниже первого слайдера
				float soundSliderWidth = 300 * scaleX;
				Slider soundEffectsSlider(soundSliderX, soundSliderY, soundSliderWidth, 0, 100, &soundEffectsVolume);

				// Текст для звуковых эффектов
				Text soundEffectsText("", font, 30);
				soundEffectsText.setPosition(350 * scaleX, 440 * scaleY);
				soundEffectsText.setFillColor(sf::Color::White);

				// Текст значения для звуковых эффектов
				Text soundEffectsValueText;
				soundEffectsValueText.setFont(font);
				soundEffectsValueText.setCharacterSize(30);
				soundEffectsValueText.setFillColor(sf::Color::White);
				soundEffectsValueText.setPosition(910 * scaleX, 555 * scaleY);

				// Вложенный цикл настроек
				while (showSettings) {
					Event event;
					while (window.pollEvent(event)) {
						// Обработка закрытия окна
						if (event.type == Event::Closed) {
							window.close();
							return 0;
						}

						// Обработка событий слайдеров
						volumeSlider.handleEvent(event, window);
						soundEffectsSlider.handleEvent(event, window);
						// Выход по ESC
						if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape) {
							showSettings = false;
							// Сохранение настроек
							saveGame(savedLevel, savedScore, Player::deathCount,
								collectedCoinsMap, currentSkin,
								item1Purchased, item3Purchased, globalVolume);
						}

						// Обработка изменения размера окна
						if (event.type == Event::Resized) {
							windowSize = window.getSize();
							FloatRect visibleArea(0, 0, event.size.width, event.size.height);
							window.setView(View(visibleArea));

							// Пересчет масштаба
							scaleX = static_cast<float>(windowSize.x) / settingsMenuTexture.getSize().x;
							scaleY = static_cast<float>(windowSize.y) / settingsMenuTexture.getSize().y;
							settingsMenuSprite.setScale(scaleX, scaleY);

							// Обновление позиций элементов
							sliderX = 500 * scaleX;
							sliderY = 400 * scaleY;
							sliderWidth = 300 * scaleX;

							// Обновление слайдера
							volumeSlider.setPosition(sliderX, sliderY);
							volumeSlider.setWidth(sliderWidth);
							volumeSlider.updateThumbPosition();

							// Обновление позиций текста
							volumeText.setPosition(350 * scaleX, 390 * scaleY);
							volumeValueText.setPosition(820 * scaleX, 390 * scaleY);
						}
					}

					// Обновление значений текста
					volumeValueText.setString(std::to_string(static_cast<int>(globalVolume)) + "%");
					soundEffectsValueText.setString(std::to_string(static_cast<int>(soundEffectsVolume)) + "%");
					// Отрисовка экрана настроек
					window.clear();
					window.draw(settingsMenuSprite);
					window.draw(volumeText);
					window.draw(volumeValueText);
					window.draw(soundEffectsText);
					window.draw(soundEffectsValueText);
					soundEffectsSlider.draw(window);
					volumeSlider.draw(window);
					window.display();
				}
			}
		}

		// Основная обработка меню
		// Сброс текстур кнопок на стандартные
		menu1.setTexture(menuTexture1);
		menu2.setTexture(menuTexture2);
		menu4.setTexture(menuTexture4);
		menu3.setTexture(menuTexture3);
		menu6.setTexture(menuTexture6);
		menu7.setTexture(menuTexture7);
		menu8.setTexture(menuTexture8); // ДОБАВЛЕНО
		// Для кнопки продолжения (если есть сохранение)
		if (hasSave) menu5.setTexture(menuTexture5);

		// Получение позиции курсора
		Vector2i mousePos = Mouse::getPosition(window);
		// Сброс номера меню
		menuNum = 0;
		// Текущая кнопка под курсором
		Sprite* currentHoveredButton = nullptr;

		// Проверка наведения на кнопки
		int counter = 1;
		// Проверка кнопки Continue
		if (hasSave && menu5.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu5.setTexture(menuTexture5_h);
			menuNum = 1;
			currentHoveredButton = &menu5;
		}
		// Проверка кнопки New Game
		else if (menu1.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu1.setTexture(menuTexture1_h);
			menuNum = hasSave ? 2 : 1;
			currentHoveredButton = &menu1;
		}
		// Проверка кнопки About Author
		else if (menu2.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu2.setTexture(menuTexture2_h);
			menuNum = hasSave ? 3 : 2;
			currentHoveredButton = &menu2;
		}
		// Проверка кнопки About Game
		else if (menu4.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu4.setTexture(menuTexture4_h);
			menuNum = hasSave ? 4 : 3;
			currentHoveredButton = &menu4;
		}
		// Проверка кнопки Shop
		else if (menu6.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu6.setTexture(menuTexture6_h);
			menuNum = hasSave ? 5 : 4;
			currentHoveredButton = &menu6;
		}
		// Проверка кнопки Settings
		else if (menu7.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu7.setTexture(menuTexture7_h);
			menuNum = hasSave ? 6 : 5;
			currentHoveredButton = &menu7;
		}
		// Проверка кнопки Exit
		else if (menu3.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu3.setTexture(menuTexture3_h);
			menuNum = hasSave ? 7 : 6;
			currentHoveredButton = &menu3;
		}
		// Проверка кнопки Help
		else if (menu8.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu8.setTexture(menuTexture8_h);
			menuNum = hasSave ? 8 : 7; // Устанавливаем уникальный номер для справки
			currentHoveredButton = &menu8;
		}

		// Воспроизведение звука при наведении
		if (!showAbout && !showAboutGame && !showShop && !showSettings) {
			// Сброс состояния после выхода из подменю
			if (wasInSubMenu) {
				prevHoveredButton = nullptr;
				wasInSubMenu = false;
			}

			// Воспроизведение звука при смене кнопки
			if (currentHoveredButton != nullptr && currentHoveredButton != prevHoveredButton) {
				hoverSound.play();
			}
		}
		else {
			// Установка флага нахождения в подменю
			wasInSubMenu = true;
		}

		// Обновление предыдущей кнопки
		prevHoveredButton = currentHoveredButton;

		static bool helpButtonPressed = false; // Флаг состояния кнопки справки

		//для того чтобы не зажималась справка
		if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
		
			helpButtonPressed = false;
		}
		// Обработка кликов мышью
		if (Mouse::isButtonPressed(Mouse::Left)) {
			// Обработка для случая с сохранением
			if (hasSave) {
				switch (menuNum) {
				case 1: return 2; // Continue
				case 2: return 1; // New Game
				case 3: showAbout = true; break; // Автор
				case 4: showAboutGame = true; break; // Об игре
				case 5: showShop = true; break; // Магазин
				case 6: showSettings = true; break; // Настройки
				case 7: window.close(); return 0; // Выход
				case 8: if (!helpButtonPressed) {
					system("start Ballu.chm");
					helpButtonPressed = true; // Блокируем повторное открытие
				}
					  break;
				}
			}
			// Обработка без сохранения
			else {
				switch (menuNum) {
				case 1: return 1; // New Game
				case 2: showAbout = true; break; // Автор
				case 3: showAboutGame = true; break; // Об игре
				case 4: showShop = true; break; // Магазин
				case 5: showSettings = true; break; // Настройки
				case 6: window.close(); return 0; // Выход
				case 7:  if (!helpButtonPressed) {
					system("start Ballu.chm");
					helpButtonPressed = true; // Блокируем повторное открытие
				}
					  break;
				}
			}
		}

		// Отрисовка основного меню
		// Масштабирование фона меню
		menuBg.setScale(
			static_cast<float>(windowSize.x) / menuBackground.getSize().x,
			static_cast<float>(windowSize.y) / menuBackground.getSize().y
		);

		// Очистка окна
		window.clear();
		// Отрисовка фона
		window.draw(menuBg);
		// Отрисовка кнопок (если доступны)
		if (hasSave) window.draw(menu5);
		window.draw(menu1);
		window.draw(menu2);
		window.draw(menu4);
		window.draw(menu6);
		window.draw(menu7);
		window.draw(menu8); // Кнопка справки
		window.draw(menu3);
		// Отображение на экране
		window.display();
	}

	// Восстановление начального вида
	window.setView(initialView);
	// Возврат значения по умолчанию
	return 0;
}

////////////////////////////////////////// ПОРТАЛ ///////////////////////////////
class Portal : public Entity {  // Объявление класса Portal, наследующего Entity
private:                        // Секция приватных членов класса
	float targetX;              // Целевая позиция по оси X для перемещения
	float startX;               // Начальная позиция по оси X
	float activationXStart;     // Начало зоны активации по X (левая граница)
	float activationXEnd;       // Конец зоны активации по X (правая граница)
	float speed;                // Скорость перемещения портала
	bool isActivated;           // Флаг активации портала (начал ли движение)
	bool isAtTarget;            // Флаг достижения целевой позиции

public:                         // Секция публичных методов класса
	// Конструктор класса Portal
	Portal(Image& image, String Name, Level& lvl, float X, float Y, int W, int H,
		float tX, float actXStart, float actXEnd, float spd)
		: Entity(image, Name, X, Y, W, H), // Вызов конструктора базового класса Entity
		targetX(tX),                       // Инициализация целевой позиции X
		startX(X),                         // Инициализация стартовой позиции X
		activationXStart(actXStart),       // Инициализация начала зоны активации
		activationXEnd(actXEnd),           // Инициализация конца зоны активации
		speed(spd),                        // Инициализация скорости перемещения
		isActivated(false),                // Инициализация флага активации (false)
		isAtTarget(false)                  // Инициализация флага достижения цели (false)
	{
		sprite.setTextureRect(IntRect(0, 0, W, H)); // Установка прямоугольника текстуры спрайта
		sprite.setOrigin(w / 2, h / 2);             // Установка центра спрайта
	}

	// Метод проверки активации портала
	void checkActivation(float playerX) {
		// Проверка: если портал не активирован и игрок в зоне активации
		if (!isActivated && playerX >= activationXStart && playerX <= activationXEnd) {
			isActivated = true; // Активировать портал
		}
	}

	// Метод обновления состояния портала (переопределение виртуального метода)
	void update(float time) override {
		// Если портал активирован и еще не достиг цели
		if (isActivated && !isAtTarget) {
			float step = speed * time; // Расчет шага перемещения за время time

			// Движение вправо (если цель справа от старта)
			if (targetX > startX) {
				x += step;             // Увеличение позиции X
				if (x >= targetX) {    // Проверка достижения цели
					x = targetX;       // Фиксация на целевой позиции
					isAtTarget = true; // Установка флага достижения
				}
			}
			// Движение влево (если цель слева от старта)
			else {
				x -= step;             // Уменьшение позиции X
				if (x <= targetX) {    // Проверка достижения цели
					x = targetX;       // Фиксация на целевой позиции
					isAtTarget = true; // Установка флага достижения
				}
			}
		}
		sprite.setPosition(x + w / 2, y + h / 2); // Обновление позиции спрайта
	}

	// Метод проверки готовности портала
	bool isPortalReady() const {
		return isAtTarget; // Возвращает состояние достижения цели
	}

	// Метод получения центра портала
	sf::Vector2f getCenter() const {
		return sf::Vector2f(x + w / 2, y + h / 2); // Вычисление координат центра
	}
};

// Класс падающей платформы
class failure : public Entity { // Объявление класса failure, наследующего Entity
private:                        // Секция приватных членов класса
	bool isActivated;           // Флаг активации падения
	float activationX;          // X-координата активации падения
	float startY;               // Начальная позиция по Y (для сброса)
	Sound fallSound;            // Звуковой эффект падения

public:                         // Секция публичных методов класса
	// Конструктор класса failure
	failure(Image& image, String Name, Level& lvl, float X, float Y, int W, int H, float actX, SoundBuffer& fallBuffer)
		: Entity(image, Name, X, Y, W, H), // Вызов конструктора базового класса
		activationX(actX),               // Инициализация X-координаты активации
		startY(Y),                       // Инициализация стартовой позиции Y
		fallSound(fallBuffer)            // Инициализация звука падения
	{
		// Тело конструктора пустое
	}

	// Геттер состояния активации
	bool isActivatedPlatform() const {
		return isActivated; // Возврат флага активации
	}

	// Геттер координаты активации
	float getActivationX() const {
		return activationX; // Возврат X-координаты активации
	}

	// Метод активации падения
	void activate() {
		if (!isActivated) {               // Если платформа не активирована
			isActivated = true;            // Установка флага активации
			dy = 0.42f;                   // Задание вертикальной скорости
			fallSound.setVolume(soundEffectsVolume); // Установка громкости звука
			fallSound.play();             // Воспроизведение звука падения
		}
	}

	// Метод обновления состояния платформы
	void update(float time) override {
		if (!isActivated) {
			// Логика до активации отсутствует
		}
		else {
			y += dy * time; // Обновление позиции Y с учетом скорости
		}
		sprite.setPosition(x + w / 2, y + h / 2); // Обновление позиции спрайта
	}
};

// Класс сложной движущейся платформы
class HardPlatform : public Entity { // Объявление класса HardPlatform
private:                            // Секция приватных членов
	float targetX;                  // Целевая позиция X
	float targetY;                  // Целевая позиция Y
	// Перечисление состояний движения
	enum MoveState { MovingX, MovingY, Done } moveState;
	bool moveOrderXY;               // Порядок движения: true = X->Y, false = Y->X
	bool isActivated;               // Флаг активации платформы
	float activationXStart;         // Начало зоны активации по X
	float activationXEnd;           // Конец зоны активации по X
	float activationYStart;         // Начало зоны активации по Y
	float activationYEnd;           // Конец зоны активации по Y
	float speedX;                   // Скорость движения по оси X
	float speedY;                   // Скорость движения по оси Y
	Sound hardSound;                // Звук движения платформы

public:                             // Секция публичных методов
	// Конструктор класса HardPlatform
	HardPlatform(Image& image, String Name, Level& lvl,
		float X, float Y, int W, int H,
		float tX, float tY, bool moveXY,
		float actXStart, float actXEnd,
		float actYStart, float actYEnd,
		float spdX, float spdY,
		SoundBuffer& hardBuffer)
		: Entity(image, Name, X, Y, W, H), // Инициализация базового класса
		targetX(tX),                     // Инициализация целевой X
		targetY(tY),                     // Инициализация целевой Y
		moveOrderXY(moveXY),             // Инициализация порядка движения
		moveState(Done),                 // Инициализация состояния (начально Done)
		isActivated(false),              // Инициализация флага активации
		activationXStart(actXStart),     // Инициализация старта активации X
		activationXEnd(actXEnd),         // Инициализация конца активации X
		activationYStart(actYStart),     // Инициализация старта активации Y
		activationYEnd(actYEnd),         // Инициализация конца активации Y
		speedX(spdX),                    // Инициализация скорости по X
		speedY(spdY),                    // Инициализация скорости по Y
		hardSound(hardBuffer)            // Инициализация звукового эффекта
	{
		sprite.setTextureRect(IntRect(0, 0, W, H)); // Настройка текстуры спрайта
		dx = 0;                     // Инициализация скорости по X = 0
		dy = 0;                     // Инициализация скорости по Y = 0
		hardSound.setVolume(50.f);   // Установка базовой громкости звука
	}

	// Метод активации движения платформы
	void activate() {
		if (!isActivated) {          // Если платформа не активирована
			isActivated = true;       // Установка флага активации
			hardSound.setVolume(soundEffectsVolume); // Обновление громкости
			hardSound.play();         // Воспроизведение звука

			// Выбор направления движения в зависимости от порядка
			if (moveOrderXY) {        // Если порядок X->Y
				moveState = MovingX;  // Установка состояния "Движение по X"
				dx = (targetX > x) ? speedX : -speedX; // Расчет направления по X
				dy = 0;               // Обнуление скорости по Y
			}
			else {                    // Если порядок Y->X
				moveState = MovingY;  // Установка состояния "Движение по Y"
				dy = (targetY > y) ? speedY : -speedY; // Расчет направления по Y
				dx = 0;               // Обнуление скорости по X
			}
		}
	}

	// Метод обновления состояния платформы
	void update(float time) override {
		if (isActivated) {            // Если платформа активирована
			switch (moveState) {      // Выбор логики по текущему состоянию
			case MovingX:             // Состояние: движение по X
				// Проверка направления и достижения цели
				if ((dx > 0 && x < targetX) || (dx < 0 && x > targetX)) {
					x += dx * time;   // Изменение позиции X
				}
				else {
					x = targetX;      // Фиксация на цели X
					dx = 0;           // Сброс скорости X
					// Выбор следующего состояния
					if (moveOrderXY) {
						moveState = MovingY; // Переход к движению по Y
						dy = (targetY > y) ? speedY : -speedY; // Направление по Y
					}
					else {
						moveState = Done; // Окончание движения
					}
				}
				break;

			case MovingY:             // Состояние: движение по Y
				// Проверка направления и достижения цели
				if ((dy > 0 && y < targetY) || (dy < 0 && y > targetY)) {
					y += dy * time;   // Изменение позиции Y
				}
				else {
					y = targetY;      // Фиксация на цели Y
					dy = 0;          // Сброс скорости Y
					// Выбор следующего состояния
					if (!moveOrderXY) {
						moveState = MovingX; // Переход к движению по X
						dx = (targetX > x) ? speedX : -speedX; // Направление по X
					}
					else {
						moveState = Done; // Окончание движения
					}
				}
				break;

			case Done:                // Состояние: движение завершено
				break;                // Никаких действий
			}
		}
		sprite.setPosition(x + w / 2, y + h / 2); // Обновление позиции спрайта
	}

	// Геттеры зон активации
	float getActivationXStart() const {
		return activationXStart; // Возврат начала зоны X
	}
	float getActivationXEnd() const {
		return activationXEnd;   // Возврат конца зоны X
	}
	float getActivationYStart() const {
		return activationYStart; // Возврат начала зоны Y
	}
	float getActivationYEnd() const {
		return activationYEnd;   // Возврат конца зоны Y
	}

	// Метод проверки активности платформы
	bool isActive() const {
		return isActivated; // Возврат состояния активации
	}
};
//////////////////////////////// КЛАСС SPIKE /////////////////////////////
class Spike : public Entity {  // Объявление класса Spike, наследующего от Entity
private:  // Секция закрытых членов класса
	float startPos;        // Стартовая координата шипа (X/Y) перед активацией
	float targetPos;       // Конечная координата движения шипа
	bool isMovingX;        // Флаг направления движения (true - горизонтальное, false - вертикальное)
	bool isActivated;      // Флаг состояния активации шипа
	float activationXStart, activationXEnd; // Границы зоны активации по оси X
	float activationYStart, activationYEnd; // Границы зоны активации по оси Y
	float platformSpeed;   // Скорость перемещения шипа
	Sound spikeSound;      // Звуковой объект для эффекта активации

public:  // Секция открытых методов класса
	// Конструктор класса Spike с параметрами
	Spike(Image& image, String Name, Level& lvl, float X, float Y,
		int W, int H, float tPos, bool moveX,
		float actXStart, float actXEnd,
		float actYStart, float actYEnd,
		float speed, SoundBuffer& spikeBuffer)
		: Entity(image, Name, X, Y, W, H), // Вызов конструктора базового класса
		startPos(moveX ? X : Y),        // Инициализация стартовой позиции (X или Y)
		targetPos(tPos),                // Инициализация целевой позиции
		isMovingX(moveX),               // Установка направления движения
		isActivated(false),             // Изначальное состояние - не активирован
		activationXStart(actXStart),    // Инициализация левой границы зоны активации X
		activationXEnd(actXEnd),        // Инициализация правой границы зоны активации X
		activationYStart(actYStart),    // Инициализация верхней границы зоны активации Y
		activationYEnd(actYEnd),        // Инициализация нижней границы зоны активации Y
		platformSpeed(speed),           // Установка скорости движения
		spikeSound(spikeBuffer)         // Инициализация звука из буфера
	{  // Начало тела конструктора
		sprite.setTextureRect(IntRect(0, 0, W, H)); // Установка прямоугольника текстуры спрайта
		dx = 0; // Инициализация горизонтальной скорости нулем
		dy = 0; // Инициализация вертикальной скорости нулем
	}  // Конец тела конструктора

	// Метод получения левой границы активации по X
	float getActivationXStart() const { return activationXStart; }
	// Метод получения правой границы активации по X
	float getActivationXEnd() const { return activationXEnd; }
	// Метод получения верхней границы активации по Y
	float getActivationYStart() const { return activationYStart; }
	// Метод получения нижней границы активации по Y
	float getActivationYEnd() const { return activationYEnd; }
	// Метод проверки активности шипа
	bool isActive() const { return isActivated; }

	// Метод активации движения шипа
	void activate() {
		if (!isActivated) {               // Проверка, не активирован ли уже шип
			isActivated = true;           // Установка флага активации
			spikeSound.play();            // Воспроизведение звука активации
			spikeSound.setVolume(50.f);   // Установка громкости звука (50%)

			// Выбор целевой координаты в зависимости от направления
			float* currentPos = isMovingX ? &x : &y;
			// Расчет направления скорости (к цели или от)
			float speed = (targetPos > *currentPos) ? platformSpeed : -platformSpeed;

			if (isMovingX) dx = speed;    // Установка скорости по X если горизонтальное движение
			else dy = speed;              // Установка скорости по Y если вертикальное движение
		}
	}

	// Метод обновления состояния шипа (переопределение виртуального метода)
	void update(float time) override {
		if (isActivated) {  // Проверка активации шипа
			if (isMovingX) {  // Обработка горизонтального движения
				// Проверка необходимости продолжать движение по X
				if ((dx > 0 && x < targetPos) || (dx < 0 && x > targetPos)) {
					x += dx * time;       // Обновление позиции по X
				}
				else {  // Достигнута целевая позиция
					x = targetPos;        // Фиксация на конечной позиции
					dx = 0;               // Сброс скорости
				}
			}
			else {  // Обработка вертикального движения
				// Проверка необходимости продолжать движение по Y
				if ((dy > 0 && y < targetPos) || (dy < 0 && y > targetPos)) {
					y += dy * time;       // Обновление позиции по Y
				}
				else {  // Достигнута целевая позиция
					y = targetPos;        // Фиксация на конечной позиции
					dy = 0;               // Сброс скорости
				}
			}
		}
		// Обновление позиции спрайта (центрирование)
		sprite.setPosition(x + w / 2, y + h / 2);
	}
};  // Конец объявления класса Spike

// Объявление класса Coin (монета)
class Coin : public Entity {  // Наследование от Entity
private:  // Закрытые члены класса
	float CurrentFrame; // Текущий индекс кадра анимации
	float frameTime;    // Время отображения одного кадра в секундах
	float timer;        // Счетчик времени для анимации

public:  // Открытые методы
	// Конструктор класса Coin
	Coin(Image& image, String Name, Level& lvl, float X, float Y, int W, int H)
		: Entity(image, Name, X, Y, W, H) {  // Вызов конструктора базового класса
		sprite.setTextureRect(IntRect(0, 0, W, H)); // Установка начального прямоугольника текстуры
		CurrentFrame = 0; // Начальный кадр анимации
		frameTime = 0.1f; // Интервал между кадрами (0.1 секунды)
		timer = 0;        // Сброс счетчика времени
	}  // Конец конструктора

	// Метод обновления состояния монеты
	void update(float time) override {
		timer += time / 1000.0f; // Преобразование миллисекунд в секунды

		// Проверка необходимости смены кадра
		if (timer >= frameTime) {
			CurrentFrame += 1; // Переход к следующему кадру
			if (CurrentFrame >= 7) CurrentFrame = 0; // Зацикливание анимации (0-6 кадры)
			timer = 0; // Сброс таймера

			// Обновление текстуры спрайта для анимации
			sprite.setTextureRect(IntRect(
				32 * static_cast<int>(CurrentFrame), // Расчет смещения по X (32px на кадр)
				0,                                   // Y-координата в текстуре
				32,                                  // Ширина кадра
				32                                   // Высота кадра
			));
		}

		// Центрирование спрайта на позиции объекта
		sprite.setPosition(x + w / 2, y + h / 2);
	}
};  // Конец объявления класса Coin

// Функция смены игрового уровня
void changeLevel(Level& lvl, int& numberLevel) {
	// Загрузка уровня в зависимости от номера
	if (numberLevel == 1) lvl.LoadFromFile("map/lvl1.tmx");  // Загрузка 1 уровня
	else if (numberLevel == 2) lvl.LoadFromFile("map/lvl2.tmx");  // Загрузка 2 уровня
	else if (numberLevel == 3) lvl.LoadFromFile("map/lvl3.tmx");  // Загрузка 3 уровня
	else if (numberLevel == 4) lvl.LoadFromFile("map/lvl4.tmx");  // Загрузка 4 уровня
	else if (numberLevel == 5) lvl.LoadFromFile("map/lvl5.tmx");  // Загрузка 5 уровня
	else if (numberLevel == 6) lvl.LoadFromFile("map/lvl6.tmx");  // Загрузка 6 уровня
	else if (numberLevel == 7) lvl.LoadFromFile("map/lvl7.tmx");  // Загрузка 7 уровня
	else if (numberLevel == 8) lvl.LoadFromFile("map/lvl8.tmx");  // Загрузка 8 уровня
	else if (numberLevel == 9) lvl.LoadFromFile("map/lvl9.tmx");  // Загрузка 9 уровня
	else if (numberLevel == 10) lvl.LoadFromFile("map/lvl10.tmx"); // Загрузка 10 уровня
	else if (numberLevel == 11) lvl.LoadFromFile("map/lvl11.tmx"); // Загрузка 11 уровня
	else if (numberLevel == 12) lvl.LoadFromFile("map/lvl12.tmx"); // Загрузка 12 уровня
	else if (numberLevel == 13) lvl.LoadFromFile("map/lvl13.tmx"); // Загрузка 13 уровня
	else if (numberLevel == 14) lvl.LoadFromFile("map/lvl14.tmx"); // Загрузка 14 уровня
	else if (numberLevel == 15) lvl.LoadFromFile("map/lvl15.tmx"); // Загрузка 15 уровня
	// Получение размеров уровня из Tiled-карты
	sf::Vector2f levelSize = lvl.GetLevelSize();
	// Установка параметров игровой камеры под размер уровня
	setLevelView(levelSize);
}  // Конец функции changeLevel

// Глобальный объект для фоновой музыки игры
Music gameMusic;
// Объявление функции запуска игры
int startGame(RenderWindow& window, int& numberLevel, int& currentScore, const std::string& currentSkin) {
	// Установка ограничения FPS для плавной анимации
	window.setFramerateLimit(120);

	// Проверка статуса фоновой музыки
	if (gameMusic.getStatus() != Music::Playing) {
		// Загрузка файла фоновой музыки
		gameMusic.openFromFile("music/na_fone.wav");
		// Включение зацикливания музыки
		gameMusic.setLoop(true);
		// Установка громкости музыки
		gameMusic.setVolume(globalVolume);
		// Запуск воспроизведения музыки
		gameMusic.play();
	}

	// Объявление объекта шрифта
	sf::Font font;
	// Попытка загрузки шрифта из файла
	if (!font.loadFromFile("Text.ttf")) {
		// Вывод ошибки в консоль при неудаче
		std::cerr << "Ошибка загрузки шрифта Text.ttf!" << std::endl;
		// Возврат false при ошибке
		return false;
	}

	// Создание текста для отображения уровня
	sf::Text levelText;
	// Установка шрифта для текста
	levelText.setFont(font);
	// Установка размера символов
	levelText.setCharacterSize(20);
	// Установка цвета текста
	levelText.setFillColor(sf::Color::White);
	// Установка позиции текста
	levelText.setPosition(35, 32);

	// Создание текста для счетчика смертей
	sf::Text deathText;
	// Установка шрифта
	deathText.setFont(font);
	// Установка размера символов
	deathText.setCharacterSize(20);
	// Установка цвета текста
	deathText.setFillColor(sf::Color::White);
	// Установка позиции в правой части экрана
	deathText.setPosition(view.getSize().x - 150, 30);

	// Создание текста для отображения очков
	sf::Text pointsText;
	// Установка шрифта
	pointsText.setFont(font);
	// Установка размера символов
	pointsText.setCharacterSize(20);
	// Установка цвета текста
	pointsText.setFillColor(sf::Color::White);
	// Установка позиции
	pointsText.setPosition(view.getSize().x - 40, 50);

	// Сброс параметров игровой камеры
	view.reset(FloatRect(0, 0, 1276, 768));
	// Создание объекта уровня
	Level lvl;
	// Загрузка данных уровня
	changeLevel(lvl, numberLevel);

	// Объявление буфера звука прыжка
	SoundBuffer jumpBuffer;
	// Загрузка звукового файла прыжка
	jumpBuffer.loadFromFile("music/jump.wav");
	// Создание звукового объекта прыжка
	Sound jump(jumpBuffer);

	// Объявление буфера звука смерти
	SoundBuffer deadBuffer;
	// Загрузка звукового файла смерти
	deadBuffer.loadFromFile("music/dead.wav");

	// Объявление буфера звука активации платформ
	SoundBuffer hardBuffer;
	// Загрузка звукового файла
	hardBuffer.loadFromFile("music/hard.wav");
	// Создание звукового объекта
	Sound hardSound;

	// Объявление буфера звука падения платформы
	SoundBuffer fallBuffer;
	// Загрузка звукового файла
	fallBuffer.loadFromFile("music/fall.wav");
	// Создание звукового объекта
	Sound fallSound;

	// Объявление буфера звука монетки
	SoundBuffer coinBuffer;
	// Загрузка звукового файла
	coinBuffer.loadFromFile("music/coin.wav");
	// Создание звукового объекта
	Sound coinSound;
	// Привязка буфера к звуку
	coinSound.setBuffer(coinBuffer);
	// Установка громкости
	coinSound.setVolume(50.f);

	// Установка громкости звука монетки
	coinSound.setVolume(soundEffectsVolume);
	// Установка громкости звука падения
	fallSound.setVolume(soundEffectsVolume);
	// Установка громкости звука активации
	hardSound.setVolume(soundEffectsVolume);

	// Объявление изображения персонажа
	Image heroImage;
	// Загрузка изображения скина
	heroImage.loadFromFile("images/" + currentSkin);

	// Объявление изображения шипов
	Image spikeImage;
	// Загрузка изображения шипов
	spikeImage.loadFromFile("images/spike.png");

	// Объявление изображения портала
	Image portalImage;
	// Загрузка изображения портала
	portalImage.loadFromFile("images/portalmy2.png");

	// Объявление изображения разрушающейся платформы
	Image failureImage;
	// Загрузка изображения платформы
	failureImage.loadFromFile("images/failure.png");

	// Объявление изображения монетки
	Image coinImage;
	// Загрузка изображения монетки
	coinImage.loadFromFile("images/coin.png");
	// Удаление фона по ключевому цвету
	coinImage.createMaskFromColor(Color(255, 0, 255));

	// Объявление текстур кнопок
	Texture continueTexture, continueTexture_h;
	Texture menuTexture, menuTexture_h;
	Texture exitTexture, exitTexture_h;

	// Загрузка обычной текстуры кнопки продолжения
	continueTexture.loadFromFile("images/Continue.png");
	// Загрузка текстуры кнопки продолжения при наведении
	continueTexture_h.loadFromFile("images/Continueh.png");

	// Загрузка обычной текстуры кнопки меню
	menuTexture.loadFromFile("images/Menubtn.png");
	// Загрузка текстуры кнопки меню при наведении
	menuTexture_h.loadFromFile("images/Menubtnh.png");

	// Загрузка обычной текстуры кнопки выхода
	exitTexture.loadFromFile("images/Exit.png");
	// Загрузка текстуры кнопки выхода при наведении
	exitTexture_h.loadFromFile("images/Exith.png");

	// Создание спрайтов кнопок
	Sprite continueBtn(continueTexture);
	Sprite menuBtn(menuTexture);
	Sprite exitBtn(exitTexture);

	// Установка масштаба кнопки продолжения
	continueBtn.setScale(0.5f, 0.5f);
	// Установка масштаба кнопки меню
	menuBtn.setScale(0.5f, 0.5f);
	// Установка масштаба кнопки выхода
	exitBtn.setScale(0.5f, 0.5f);

	// Флаг паузы
	bool isPaused = false;
	// Объект для затемнения фона
	RectangleShape overlay;

	// Получение объекта игрока из уровня
	Object player = lvl.GetObject("player");
	// Создание объекта игрока
	Player p(heroImage, "Player1", lvl, player.rect.left, player.rect.top,
		37, 50, jumpBuffer, deadBuffer, numberLevel, currentScore);

	// Список игровых объектов
	std::list<Entity*> entities;
	// Итератор для работы со списком
	std::list<Entity*>::iterator it;
	// Второй итератор
	std::list<Entity*>::iterator it2;

	// Получение объектов порталов
	std::vector<Object> portals = lvl.GetObjects("portal");
	// Обработка порталов
	for (auto& obj : portals) {
		// Парсинг параметров портала
		float targetX = obj.properties.count("targetX") ? std::stof(obj.properties.at("targetX")) : obj.rect.left;
		float activationXStart = obj.properties.count("activationXStart") ? std::stof(obj.properties.at("activationXStart")) : obj.rect.left - 100;
		float activationXEnd = obj.properties.count("activationXEnd") ? std::stof(obj.properties.at("activationXEnd")) : obj.rect.left + 100;
		float speed = obj.properties.count("speed") ? std::stof(obj.properties.at("speed")) : 0.15f;

		// Создание и добавление портала
		entities.push_back(new Portal(
			portalImage, "portal", lvl,
			obj.rect.left, obj.rect.top,
			obj.rect.width, obj.rect.height,
			targetX, activationXStart, activationXEnd, speed
		));
	}

	// Получение объектов монеток
	std::vector<Object> coins = lvl.GetObjects("coin");
	// Обработка монеток
	for (auto& coinObj : coins) {
		// Получение координат монетки
		float x = coinObj.rect.left;
		float y = coinObj.rect.top;
		// Проверка сбора монетки
		if (collectedCoinsMap[numberLevel].find({ x, y }) == collectedCoinsMap[numberLevel].end()) {
			// Создание и добавление монетки
			entities.push_back(new Coin(coinImage, "coin", lvl, x, y, 32, 32));
		}
	}

	// Получение объектов разрушающихся платформ
	std::vector<Object> failures = lvl.GetObjects("failure");
	// Обработка платформ
	for (int i = 0; i < failures.size(); i++) {
		// Получение объекта платформы
		Object obj = failures[i];
		// Координата активации
		float activationX = 0.0f;
		// Парсинг параметра активации
		if (obj.properties.count("activationX")) {
			try {
				// Конвертация строки в число
				activationX = std::stof(obj.properties["activationX"]);
			}
			catch (const std::exception& e) {
				// Обработка ошибки конвертации
				std::cerr << "Ошибка чтения activationX: " << e.what() << std::endl;
			}
		}
		// Создание и добавление платформы
		entities.push_back(new failure(failureImage, "failure", lvl,
			obj.rect.left, obj.rect.top, 64, 32, activationX, fallBuffer));
	}

	// Получение объектов сложных платформ
	std::vector<Object> hardPlatforms = lvl.GetObjects("hard");
	// Обработка платформ
	for (auto& obj : hardPlatforms) {
		// Парсинг параметров скорости
		float speed = obj.properties.count("speed") ? std::stof(obj.properties.at("speed")) : 0.1f;
		float speedX = obj.properties.count("speedX") ? std::stof(obj.properties.at("speedX")) : speed;
		float speedY = obj.properties.count("speedY") ? std::stof(obj.properties.at("speedY")) : speed;
		// Стартовые координаты
		float startX = obj.rect.left;
		float startY = obj.rect.top;
		// Целевые координаты
		float targetX = obj.properties.count("targetX") ? std::stof(obj.properties.at("targetX")) : startX;
		float targetY = obj.properties.count("targetY") ? std::stof(obj.properties.at("targetY")) : startY;
		// Порядок движения
		bool moveOrderXY = obj.properties.count("moveOrder") ? (obj.properties.at("moveOrder") == "XY") : true;

		// Парсинг параметров активации
		float activationXStart = obj.properties.count("activationXStart") ? std::stof(obj.properties.at("activationXStart")) : 0.0f;
		float activationXEnd = obj.properties.count("activationXEnd") ? std::stof(obj.properties.at("activationXEnd")) : FLT_MAX;
		float activationYStart = obj.properties.count("activationYStart") ? std::stof(obj.properties.at("activationYStart")) : 0.0f;
		float activationYEnd = obj.properties.count("activationYEnd") ? std::stof(obj.properties.at("activationYEnd")) : FLT_MAX;

		// Создание и добавление платформы
		entities.push_back(new HardPlatform(
			failureImage, "hard", lvl,
			obj.rect.left, obj.rect.top,
			obj.rect.width, obj.rect.height,
			targetX, targetY, moveOrderXY,
			activationXStart, activationXEnd,
			activationYStart, activationYEnd,
			speedX, speedY,
			hardBuffer
		));
	}

	// Получение объектов шипов
	std::vector<Object> spikePlatforms = lvl.GetObjects("spike");
	// Обработка шипов
	for (auto& obj : spikePlatforms) {
		// Флаг движения по X
		bool moveX = true;
		// Целевая координата
		float target = obj.rect.left;
		// Определение направления движения
		if (obj.properties.count("targetY")) {
			moveX = false;
			target = std::stof(obj.properties.at("targetY"));
		}
		else if (obj.properties.count("targetX")) {
			target = std::stof(obj.properties.at("targetX"));
		}

		// Парсинг параметров активации
		float activationXStart = obj.properties.count("activationXStart") ? std::stof(obj.properties.at("activationXStart")) : 0.0f;
		float activationXEnd = obj.properties.count("activationXEnd") ? std::stof(obj.properties.at("activationXEnd")) : FLT_MAX;
		float activationYStart = obj.properties.count("activationYStart") ? std::stof(obj.properties.at("activationYStart")) : 0.0f;
		float activationYEnd = obj.properties.count("activationYEnd") ? std::stof(obj.properties.at("activationYEnd")) : FLT_MAX;
		// Скорость движения
		float speed = obj.properties.count("speed") ? std::stof(obj.properties.at("speed")) : 0.1f;

		// Создание и добавление шипа
		entities.push_back(new Spike(
			spikeImage, "spike", lvl,
			obj.rect.left, obj.rect.top,
			obj.rect.width, obj.rect.height,
			target, moveX,
			activationXStart, activationXEnd,
			activationYStart, activationYEnd,
			speed, hardBuffer
		));
	}

	// Получение размера уровня
	sf::Vector2f levelSize = lvl.GetLevelSize();
	// Настройка камеры под уровень
	setLevelView(levelSize);
	// Применение камеры к окну
	window.setView(view);

	// Таймер для расчета времени кадра
	Clock clock;
	// Флаг активации платформ
	bool isPlatformActivated = false;
	// Состояние игры (0-игра, 3-меню)
	int gameState = 0;
	// Главный игровой цикл
	while (window.isOpen()) {
		// Расчет времени кадра
		float time = clock.getElapsedTime().asMicroseconds();
		// Перезапуск таймера
		clock.restart();
		// Нормализация времени
		time = time / 800;

		// Обработка событий
		Event event;
		while (window.pollEvent(event)) {
			// Закрытие окна
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// Обработка паузы по клавише ESC
		static bool escKeyWasPressed = false;
		bool escKeyIsPressed = Keyboard::isKeyPressed(Keyboard::Escape);

		// Переключение состояния паузы
		if (escKeyIsPressed && !escKeyWasPressed) {
			isPaused = !isPaused;
			// Пауза музыки
			if (isPaused) {
				gameMusic.pause();
			}
			else {
				gameMusic.play();
			}
		}
		escKeyWasPressed = escKeyIsPressed;

		// Логика игры (если не на паузе)
		if (!isPaused) {
			// Проверка активации порталов
			for (it = entities.begin(); it != entities.end(); ++it) {
				if ((*it)->name == "portal") {
					// Приведение типа к порталу
					Portal* portal = static_cast<Portal*>(*it);
					// Проверка активации по позиции игрока
					portal->checkActivation(p.x);

					// Проверка перехода через портал
					if (portal->isPortalReady() && p.getRect().contains(portal->getCenter())) {
						// Начисление очков
						p.AddScore(10);
						// Завершение уровня
						p.completeLevel();
						// Сохранение счета
						currentScore = p.playerScore;
						// Возврат кода завершения уровня
						return 1;
					}
				}
			}

			// Обновление состояния объектов
			for (it = entities.begin(); it != entities.end();) {
				// Получение объекта
				Entity* b = *it;
				// Обновление состояния
				b->update(time);

				// Удаление "мертвых" объектов
				if (b->life == false) {
					// Удаление из списка
					it = entities.erase(it);
					// Освобождение памяти
					delete b;
				}
				else {
					it++;
				}
			}

			// Сбор монеток
			for (it = entities.begin(); it != entities.end();) {
				Entity* entity = *it;
				// Проверка столкновения
				if (entity->name == "coin" && entity->getRect().intersects(p.getRect())) {
					// Начисление очков
					p.AddScore(10);
					// Добавление в собранные монетки
					collectedCoinsMap[numberLevel].insert({ entity->x, entity->y });
					// Настройка громкости
					coinSound.setVolume(soundEffectsVolume);
					// Воспроизведение звука
					coinSound.play();
					// Удаление монетки
					it = entities.erase(it);
					delete entity;
				}
				else {
					++it;
				}
			}

			// Обработка столкновений с платформами
			for (it = entities.begin(); it != entities.end(); it++) {
				if ((*it)->name == "hard" && (*it)->getRect().intersects(p.getRect())) {
					// Расчет параметров коллизии
					FloatRect platformRect = (*it)->getRect();
					float overlapLeft = p.getRect().left + p.w - platformRect.left;
					float overlapRight = platformRect.left + platformRect.width - p.getRect().left;
					float overlapTop = p.getRect().top + p.h - platformRect.top;
					float overlapBottom = platformRect.top + platformRect.height - p.getRect().top;

					// Определение направления коллизии
					bool fromLeft = std::abs(overlapLeft) < std::abs(overlapRight);
					bool fromTop = std::abs(overlapTop) < std::abs(overlapBottom);
					float minOverlapX = fromLeft ? overlapLeft : overlapRight;
					float minOverlapY = fromTop ? overlapTop : overlapBottom;

					// Корректировка позиции игрока
					if (std::abs(minOverlapX) < std::abs(minOverlapY)) {
						fromLeft ? p.x = platformRect.left - p.w : p.x = platformRect.left + platformRect.width;
					}
					else {
						if (fromTop) {
							p.y = platformRect.top - p.h;
							p.dy = 0;
							p.onGround = true;
						}
						else {
							p.y = platformRect.top + platformRect.height;
							p.dy = 0;
						}
					}
				}
			}

			
			// Проверка столкновений с шипами
			for (it = entities.begin(); it != entities.end(); it++) {
				if ((*it)->getRect().intersects(p.getRect())) {
					if ((*it)->name == "spike") {
						// Смерть игрока
						p.die();
					}
				}
			}
			// Обновление состояния игрока
			p.update(time);

			// Активация сложных платформ
			for (it = entities.begin(); it != entities.end(); ++it) {
				if ((*it)->name == "hard") {
					// Приведение типа
					HardPlatform* hard = static_cast<HardPlatform*>(*it);
					// Проверка условий активации
					if (!hard->isActive() &&
						p.x >= hard->getActivationXStart() &&
						p.x <= hard->getActivationXEnd() &&
						p.y >= hard->getActivationYStart() &&
						p.y <= hard->getActivationYEnd()) {
						// Активация платформы
						hard->activate();
					}
				}
			}

			// Активация шипов
			for (it = entities.begin(); it != entities.end(); ++it) {
				if ((*it)->name == "spike") {
					// Приведение типа
					Spike* spike = static_cast<Spike*>(*it);
					// Проверка условий активации
					if (!spike->isActive() &&
						p.x >= spike->getActivationXStart() &&
						p.x <= spike->getActivationXEnd() &&
						p.y >= spike->getActivationYStart() &&
						p.y <= spike->getActivationYEnd()) {
						// Активация шипов
						spike->activate();
					}
				}
			}

			// Проверка необходимости рестарта уровня
			if (p.needRestartLevel) {
				currentScore = p.playerScore;
				// Очистка собранных монет уровня
				collectedCoinsMap[numberLevel].clear();
				return 2;
			}
		}

		// Настройка камеры
		window.setView(view);
		// Очистка экрана цветом фона
		window.clear(Color(77, 83, 140));
		// Отрисовка уровня
		lvl.Draw(window);

		// Активация разрушающихся платформ
		for (auto it = entities.begin(); it != entities.end(); ++it) {
			if ((*it)->name == "failure") {
				// Приведение типа
				failure* plat = static_cast<failure*>(*it);
				// Проверка активации по позиции игрока
				if (!plat->isActivatedPlatform() && p.x >= plat->getActivationX()) {
					plat->activate();
				}
			}
		}

		// Глобальная активация платформ
		if (p.x >= 1000.0f && !isPlatformActivated) {
			for (auto it = entities.begin(); it != entities.end(); it++) {
				if ((*it)->name == "failure") {
					static_cast<failure*>(*it)->activate();
				}
			}
			isPlatformActivated = true;
		}

		// Отрисовка игровых объектов
		for (it = entities.begin(); it != entities.end(); it++) {
			window.draw((*it)->sprite);
		}

		// Обновление текста уровня
		levelText.setString("Level " + std::to_string(numberLevel));
		// Обновление счетчика смертей
		deathText.setString("Death " + std::to_string(Player::deathCount));
		// Отрисовка счетчика смертей
		window.draw(deathText);
		// Отрисовка текста уровня
		window.draw(levelText);
		// Отрисовка счетчика смертей
		window.draw(deathText);

		// Корректировка позиций текстов
		deathText.setPosition(view.getSize().x - 160, 30);
		pointsText.setPosition(view.getSize().x - 160, 50);

		// Обновление текста очков
		pointsText.setString("POINTS " + std::to_string(p.playerScore + p.levelScore));
		// Отрисовка очков
		window.draw(pointsText);

		// Отрисовка игрока (если видим)
		if (p.isVisible) {
			window.draw(p.sprite);
		}

		// Отрисовка меню паузы
		if (isPaused) {
			// Настройка затемнения
			overlay.setSize(Vector2f(view.getSize().x, view.getSize().y));
			overlay.setFillColor(Color(0, 0, 0, 150));
			overlay.setPosition(view.getCenter().x - view.getSize().x / 2, view.getCenter().y - view.getSize().y / 2);
			window.draw(overlay);

			// Позиционирование кнопок
			continueBtn.setPosition(view.getCenter().x - 100, view.getCenter().y - 50);
			menuBtn.setPosition(view.getCenter().x - 100, view.getCenter().y);
			exitBtn.setPosition(view.getCenter().x - 100, view.getCenter().y + 50);

			// Статические переменные для звука наведения
			static SoundBuffer hoverBuffer;
			static Sound hoverSound;
			static bool soundLoaded = false;
			static Sprite* prevHovered = nullptr;

			// Загрузка звука наведения (однократно)
			if (!soundLoaded) {
				if (hoverBuffer.loadFromFile("music/button.ogg")) {
					hoverSound.setBuffer(hoverBuffer);
					hoverSound.setVolume(30.f);
					soundLoaded = true;
				}
			}

			// Получение позиции мыши
			Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));

			// Сброс текстур кнопок
			continueBtn.setTexture(continueTexture);
			menuBtn.setTexture(menuTexture);
			exitBtn.setTexture(exitTexture);

			// Определение наведения на кнопки
			Sprite* currentHovered = nullptr;
			if (continueBtn.getGlobalBounds().contains(mousePos)) {
				continueBtn.setTexture(continueTexture_h);
				currentHovered = &continueBtn;
			}
			else if (menuBtn.getGlobalBounds().contains(mousePos)) {
				menuBtn.setTexture(menuTexture_h);
				currentHovered = &menuBtn;
			}
			else if (exitBtn.getGlobalBounds().contains(mousePos)) {
				exitBtn.setTexture(exitTexture_h);
				currentHovered = &exitBtn;
			}

			// Воспроизведение звука при наведении
			if (currentHovered != prevHovered) {
				if (currentHovered != nullptr && soundLoaded) {
					hoverSound.play();
				}
				prevHovered = currentHovered;
			}

			// Отрисовка кнопок
			window.draw(continueBtn);
			window.draw(menuBtn);
			window.draw(exitBtn);

			// Обработка кликов по кнопкам
			if (Mouse::isButtonPressed(Mouse::Left)) {
				if (continueBtn.getGlobalBounds().contains(mousePos)) {
					// Продолжение игры
					isPaused = false;
				}
				else if (menuBtn.getGlobalBounds().contains(mousePos)) {
					// Сохранение игры
					saveGame(savedLevel, savedScore, Player::deathCount,
						collectedCoinsMap, currentSkin, item1Purchased, item3Purchased, globalVolume);
					// Возврат в меню
					return 3;
				}
				else if (exitBtn.getGlobalBounds().contains(mousePos)) {
					// Закрытие игры
					window.close();
					return 0;
				}
			}
		}
		// Обновление экрана
		window.display();
	}
	// Сохранение счета при выходе
	currentScore = p.playerScore;
	// Возврат состояния игры
	return gameState;
}

// Текстура для экрана победы
Texture winTexture;
// Буфер для звука победы
SoundBuffer winSoundBuffer;
// Звуковой объект победы
Sound winSound;
// Флаг воспроизведения звука победы (чтобы не повторять)
bool winMusicPlayed = false;

// Функция основного игрового процесса
void gameRunning(RenderWindow& window, int& savedLevel, int& currentScore,
	int& savedDeaths, std::map<int, std::set<std::pair<float, float>>>& savedCoins,
	std::string& currentSkin) {
	// Сохранение оригинального вида камеры
	View originalView = window.getView();
	// Флаг продолжения игрового процесса
	bool running = true;
	// Основной игровой цикл
	while (running) {
		// Запуск уровня и получение результата
		int result = startGame(window, savedLevel, currentScore, currentSkin);
		// Восстановление оригинальной камеры
		window.setView(originalView);

		// Обработка результата уровня
		switch (result) {
		case 1: // Переход на следующий уровень
			savedLevel++;
			// Проверка завершения игры (15 уровней)
			if (savedLevel > 15) {
				// Загрузка текстуры экрана победы
				if (!winTexture.loadFromFile("images/win.png")) {
					// Ошибка загрузки текстуры
					std::cerr << "Failed to load win.png!" << std::endl;
				}
				// Загрузка звука победы
				if (!winSoundBuffer.loadFromFile("music/win.wav")) {
					// Ошибка загрузки звука
					std::cerr << "Failed to load win.wav!" << std::endl;
				}

				// Настройка звука победы
				winSound.setBuffer(winSoundBuffer);
				winSound.setLoop(false); // Однократное воспроизведение
				winSound.setVolume(globalVolume); // Глобальная громкость
				winMusicPlayed = false; // Сброс флага воспроизведения

				// Остановка фоновой музыки
				gameMusic.stop();

				// Создание спрайта победы
				Sprite winSprite(winTexture);
				// Масштабирование под размер окна
				winSprite.setScale(
					static_cast<float>(window.getSize().x) / winTexture.getSize().x,
					static_cast<float>(window.getSize().y) / winTexture.getSize().y
				);

				// Цикл отображения экрана победы
				bool winScreen = true;
				Clock winClock; // Таймер для управления
				while (winScreen && window.isOpen()) {
					Event event;
					// Обработка событий окна
					while (window.pollEvent(event)) {
						// Закрытие окна
						if (event.type == Event::Closed) {
							window.close();
							winScreen = false;
						}
						// Выход по Escape
						if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape) {
							winScreen = false;
						}
					}

					// Однократное воспроизведение звука победы
					if (!winMusicPlayed) {
						winSound.play();
						winMusicPlayed = true;
					}

					// Отрисовка экрана победы
					window.clear();
					window.draw(winSprite);
					window.display();
				}

				// Сброс игрового прогресса
				savedLevel = 1; // Уровень 1
				savedScore = 0; // Счетчик очков
				savedDeaths = 0; // Счетчик смертей
				Player::deathCount = 0; // Сброс статического счетчика
				collectedCoinsMap.clear(); // Очистка собранных монет
				currentSkin = "pers.png"; // Скин по умолчанию
				item1Purchased = false; // Сброс покупки 1
				item3Purchased = false; // Сброс покупки 3

				// Сохранение сброшенного состояния
				saveGame(savedLevel, savedScore, savedDeaths,
					collectedCoinsMap, currentSkin,
					item1Purchased, item3Purchased, globalVolume);

				// Завершение игрового цикла
				running = false;
			}
			break;
		case 2: // Перезапуск уровня
			break;
		case 3: // Возврат в меню
			savedDeaths = Player::deathCount; // Сохранение счетчика смертей
			// Сохранение прогресса
			saveGame(savedLevel, currentScore, savedDeaths, savedCoins, currentSkin,
				item1Purchased, item3Purchased, globalVolume);
			running = false; // Завершение игрового цикла
			break;
		default: // Выход
			running = false;
		}
	}
}

// Инициализация статической переменной счетчика смертей
int Player::deathCount = 0;

// Главная функция программы
int main() {
	// Создание игрового окна (полноэкранного)
	RenderWindow window(VideoMode(1376, 768), "Game", Style::Fullscreen);
	// Ограничение частоты кадров
	window.setFramerateLimit(60);

	// Показ загрузочного экрана
	showLoadingScreen(window);

	// Локальные переменные для экрана победы
	Texture winTexture;
	SoundBuffer winSoundBuffer;
	Sound winSound;
	bool winMusicPlayed = false;

	// Инициализация переменных сохранения
	int savedLevel = 1; // Текущий уровень
	int savedScore = 0; // Набранные очки
	int savedDeaths = 0; // Количество смертей
	std::map<int, std::set<std::pair<float, float>>> savedCoins; // Собранные монеты
	std::string currentSkin = "pers.png"; // Текущий скин игрока
	bool item1Purchased = false; // Статус покупки 1
	bool item3Purchased = false; // Статус покупки 3

	// Загрузка сохраненной игры
	bool hasSave = loadGame(savedLevel, savedScore, savedDeaths,
		savedCoins, currentSkin, item1Purchased, item3Purchased, globalVolume);

	// Восстановление счетчика смертей
	Player::deathCount = savedDeaths;

	// Главный цикл приложения
	while (window.isOpen()) {
		Event event;
		// Обработка событий окна
		while (window.pollEvent(event)) {
			// Событие закрытия окна
			if (event.type == Event::Closed) {
				// Сохранение перед выходом
				saveGame(savedLevel, savedScore, Player::deathCount,
					collectedCoinsMap, currentSkin, item1Purchased, item3Purchased, globalVolume);
				window.close(); // Закрытие окна
			}
		}

		// Отображение меню и получение результата
		int menuResult = menu(window, hasSave, savedScore, savedLevel,
			collectedCoinsMap, currentSkin, item1Purchased, item3Purchased);

		// Обработка выбора "Новая игра"
		if (menuResult == 1) {
			item1Purchased = false; // Сброс покупки 1
			item3Purchased = false; // Сброс покупки 3
			currentSkin = "pers.png"; // Сброс скина
			savedLevel = 1; // Начальный уровень
			savedScore = 0; // Сброс очков
			savedDeaths = 0; // Сброс смертей
			Player::deathCount = 0;  // Сброс счетчика
			collectedCoinsMap.clear(); // Очистка монет
		}
		// Обработка выбора "Продолжить"
		else if (menuResult == 2) {
			Player::deathCount = savedDeaths;  // Восстановление смертей
			collectedCoinsMap = savedCoins; // Восстановление монет
		}

		// Запуск игры если не выход
		if (menuResult != 0) {
			// Запуск игрового процесса
			gameRunning(window, savedLevel, savedScore, savedDeaths,
				savedCoins, currentSkin);
			// Сохранение после завершения игры
			saveGame(savedLevel, savedScore, Player::deathCount,
				collectedCoinsMap, currentSkin, item1Purchased, item3Purchased, globalVolume);
			// Обновление сохраненных значений
			savedDeaths = Player::deathCount;
			hasSave = true;
		}
		else {
			window.close(); // Закрытие при выходе из меню
		}
	}
	return 0; // Завершение программы
}