// ����������� ����������� ���������� SFML
#include <SFML/Graphics.hpp>
// ����������� ��������������� SFML
#include <SFML/Audio.hpp>
// ������������ ���� ��� ���������� �������
#include "view.h"
// ����������� ���������� �����/������
#include <iostream>
// ���������� ��� ������ �� ��������
#include <sstream>
// ������������ ���� � ���������/��������
#include "mission.h"
// ������������ ���� ��� ������ � ��������
#include "level.h"
// ���������� ��� ������ � ������������� ���������
#include <vector>
// ���������� ��� ������ �� ��������
#include <list>
// ���������� ��� ������ � XML-�������
#include "TinyXML/tinyxml.h"
// ������������� ������������ ���� SFML
using namespace sf;
#include <set>
#include <map>
#include <fstream>
#include <iomanip>

// ���������� ���������� ��� ���������� ���������
int savedLevel = 1;
int savedScore = 0;
int savedDeaths = 0;
bool item1Purchased = false;
bool item3Purchased = false;
std::string currentSkin = "pers.png";
std::map<int, std::set<std::pair<float, float>>> collectedCoinsMap;
//������������� �������� ��� ������ �� ������, ���������� ������ ��� ������ 

float soundEffectsVolume = 50.0f; // ��������� �������� ��������

//////////////////////////////////// �������� �����-�������� //////////////////////////
class Entity {
public:
	// ������ �������� ����� ��� ��������
	std::vector<Object> obj; // ������ �������� �����

	// ��������� �������� � �������
	float dx; // �������� �� ��� X
	float dy; // �������� �� ��� Y
	float x;  // ���������� X �������
	float y;  // ���������� Y �������
	float speed; // ����� �������� �����������
	float moveTimer; // ������ ��� �����������

	// ���������� ���������
	int w;    // ������ �������
	int h;    // ������ �������
	int health; // �������� �������

	// ����� ���������
	bool life;     // ����� �� ������
	bool isMove;   // �������� �� ������
	bool onGround; // ��������� �� �� �����

	// ����������� ����������
	Texture texture; // �������� �������
	Sprite sprite;   // ������ �������
	String name;     // ��� �������

	// ����������� ������ Entity
	Entity(Image& image, String Name, float X, float Y, int W, int H) {
		x = X; // ��������� ��������� ������� X
		y = Y; // ��������� ��������� ������� Y
		w = W; // ��������� ������
		h = H; // ��������� ������
		name = Name; // ���������� �����
		moveTimer = 0; // ��������� �������

		speed = 0;   // ��������� ��������
		health = 100; // ������ ��������
		dx = 0; // ��������� �������� �� X
		dy = 0; // ��������� �������� �� Y

		life = true;    // ������ �����
		onGround = false; // �� �� �����
		isMove = false;   // �� ��������

		texture.loadFromImage(image); // �������� ��������
		sprite.setTexture(texture);    // ��������� �������� �������
		sprite.setOrigin(w / 2, h / 2); // ������������� �������
	}

	// ����� ��������� �������� �������
	FloatRect getRect() { // �-��� ��������� ��������������
		return FloatRect(x, y, w, h); // ������� �������������� ��� ��������
	}

	// ����� ����������� ����� ���������� (������ ���� ���������� � ��������)
	virtual void update(float time) = 0;
};


////////////////////////////////////////// ����� ������ ///////////////////////////////

class Player : public Entity {
private:
	// �������� �������
	Sound jumpSound;       // ���� ������
	Sound deadSound;       // ���� ������ ��� ������������

	// ������� ������� � ����������������
	int currentLevel;      // ������� ����� ������
	Level& level;          // ������ �� ������ ������ ��� ��������������
	float startX;          // ��������� X-���������� ������
	float startY;          // ��������� Y-���������� ������

	// ������� � ���������
	sf::Clock teleportClock;       // ������ ��� �������� ������������
	bool isOutOfBounds;            // ���� ������ �� ������� ������
	static const float RESURRECTION_DELAY; // �������� ����� ������������ (0.3 ���)
	sf::Clock resurrectionClock;   // ������ ��� ������� ������� �����������
	bool isResurrecting;           // ���� ���������� ���������� ��� �����������

	// ��������
	float CurrentFrame;    // ������� ���� ��������

public:


	// ��������� ������
	enum { left, right, up, down, jump, stay } state; // ��������� ��������� ��������
	bool needRestartLevel; // ���� ������������� ����������� ������
	int playerScore;  // ����� ����
	int levelScore;   // ���� �������� ������       
	bool isVisible;        // ��������� ������ (������������ ��� ������)
	static int deathCount; // ����������� ������� �������



	void AddScore(int points) {
		levelScore += points;
	}
	// ��������� ������ ������
	void die() {
		if (!isOutOfBounds) {
			isVisible = false;
			teleportClock.restart();
			isOutOfBounds = true;
			deadSound.setVolume(soundEffectsVolume); // ��������� ���������
			deadSound.play();
			isResurrecting = true;
			resurrectionClock.restart();
			deathCount++;

			// ���������� ������� ������
			collectedCoinsMap[currentLevel].clear();
			levelScore = 0;
		}
	}
	void completeLevel() {
		playerScore += levelScore;
		levelScore = 0;
	}

	// ����������� ������
	Player(Image& image, String Name, Level& lev, float X, float Y, int W, int H,
		SoundBuffer& jumpBuffer, SoundBuffer& deadBuffer, int currentLevel, int initialScore = 0)
		: Entity(image, Name, X, Y, W, H), // ������������� �������� ������
		level(lev),               // ������������� ������ �� �������
		jumpSound(jumpBuffer),    // ������������� ����� ������
		deadSound(deadBuffer),    // ������������� ����� ������
		startX(X),                // ���������� ��������� ������� X
		startY(Y),                // ���������� ��������� ������� Y
		isOutOfBounds(false),     // ������������� ����� ������ �� �������
		isResurrecting(true),     // ��������� ��������� �����������
		CurrentFrame(0),          // ��������� ���� ��������
		currentLevel(currentLevel), // ��������� �������� ������
		playerScore(initialScore) // ������������� �����
	{
		resurrectionClock.restart(); // ������ ������� �����������
		needRestartLevel = false;    // ����� ����� ����������� ������
		state = stay;                // ��������� ��������� - ��� ��������
		obj = lev.GetObjects("solid"); // ��������� ������� �������� ������
		isVisible = true;            // ����� ����� ��� ��������
		jumpSound.setVolume(soundEffectsVolume);
		deadSound.setVolume(soundEffectsVolume);
		// ��������� ������� ��� ������� ������
		if (name == "Player1") {
			sprite.setTextureRect(IntRect(0, 0, w, h)); // ��������� ����������� ��������������
			if (health <= 0) { life = false; } // �������� ������
		}
	}
	// ��������� ����������
	void control(float time) {  // ����� ��������� ���������� �������
		sprite.setTextureRect(IntRect(0, 0, 37, 50)); // ��������� �������������� �������� ��� ������� (������� �������� �������)

		if (!isResurrecting) { // ��������, �� ��������� �� ����� � �������� �����������
			// ��������������� ���������� ��� 9 � 10 �������
			if (currentLevel == 9 || currentLevel == 10) { // �������� �������� ������
				if (Keyboard::isKeyPressed(Keyboard::Left) || Keyboard::isKeyPressed(Keyboard::A)) { // �������� ������� ������� �����
					state = right; // ��������� ��������� �������� ������ (���������������)
					speed = 0.15;  // ��������� �������� ��������
					CurrentFrame += 0.005 * time; // ���������� �������� ����� ��������
					if (CurrentFrame > 2) CurrentFrame -= 2; // ����������� ������������ ������ ��������
					sprite.setTextureRect(IntRect(39 * int(CurrentFrame), 104, 37, 50)); // ��������� �������������� �������� ��� �������� �������� ������
				}
				if (Keyboard::isKeyPressed(Keyboard::Right) || Keyboard::isKeyPressed(Keyboard::D)) { // �������� ������� ������� ������
					state = left; // ��������� ��������� �������� ����� (���������������)
					speed = 0.15; // ��������� �������� ��������
					CurrentFrame += 0.005 * time; // ���������� �������� ����� ��������
					if (CurrentFrame > 2) CurrentFrame -= 2; // ����������� ������������ ������ ��������
					sprite.setTextureRect(IntRect(39 * int(CurrentFrame), 52, 37, 50)); // ��������� �������������� �������� ��� �������� �������� �����
				}
			}
			else { // ����������� ���������� ��� ��������� �������
				if (Keyboard::isKeyPressed(Keyboard::Left) || Keyboard::isKeyPressed(Keyboard::A)) { // �������� ������� ������� �����
					state = left; // ��������� ��������� �������� �����
					speed = 0.15; // ��������� �������� ��������
					CurrentFrame += 0.005 * time; // ���������� �������� ����� ��������
					if (CurrentFrame > 2) CurrentFrame -= 2; // ����������� ������������ ������ ��������
					sprite.setTextureRect(IntRect(39 * int(CurrentFrame), 52, 37, 50)); // ��������� �������������� �������� ��� �������� �������� �����
				}
				if (Keyboard::isKeyPressed(Keyboard::Right) || Keyboard::isKeyPressed(Keyboard::D)) { // �������� ������� ������� ������
					state = right; // ��������� ��������� �������� ������
					speed = 0.15; // ��������� �������� ��������
					CurrentFrame += 0.005 * time; // ���������� �������� ����� ��������
					if (CurrentFrame > 2) CurrentFrame -= 2; // ����������� ������������ ������ ��������
					sprite.setTextureRect(IntRect(39 * int(CurrentFrame), 104, 37, 50)); // ��������� �������������� �������� ��� �������� �������� ������
				}
			}
		}

		// ��������� ������
		if ((Keyboard::isKeyPressed(Keyboard::Up)) && (onGround) || Keyboard::isKeyPressed(Keyboard::W) && (onGround)) { // �������� ������� ������� ������ � ���������� �� �����
			state = jump; // ��������� ��������� ������
			dy = -0.6;      // ��������� ������������ �������� (������� ������)
			onGround = false; // ����� ����� ���������� �� �����
			jumpSound.setVolume(soundEffectsVolume); // ��������� ��������� ����� ������
			jumpSound.play(); // ��������������� ����� ������
		}
	}

	// �������� ������������ � ������
	void checkCollisionWithMap(float Dx, float Dy) { // ����� �������� ������������ � ��������� �����
		for (int i = 0; i < obj.size(); i++) { // ���� �� ���� �������� �����
			if (getRect().intersects(obj[i].rect)) { // �������� ����������� � ������� ��������
				// ��������� ������ ����� ��������
				if (obj[i].name == "solid" || obj[i].name == "hard") { // �������� ���� ������� (�������)
					if (Dy > 0) { // ������������ �����
						y = obj[i].rect.top - h; // ��������� ������� �� Y
						dy = 0;     // ����� ������������ ��������
						onGround = true; // ��������� ����� ���������� �� �����
					}
					if (Dy < 0) { // ������������ ������
						y = obj[i].rect.top + obj[i].rect.height; // ��������� ������� �� Y
						dy = 0; // ����� ������������ ��������
					}
					if (Dx > 0) { // ������������ ������
						x = obj[i].rect.left - w; // ��������� ������� �� X
					}
					if (Dx < 0) { // ������������ �����
						x = obj[i].rect.left + obj[i].rect.width; // ��������� ������� �� X
					}
				}
			}
		}
	}

	// ���������� ��������� ������
	void update(float time) { // ����� ���������� ��������� ������
		control(time); // ����� ������ ��������� ����������

		onGround = false; // ����� ����� ���������� �� ����� � ������ �����

		// ��������� �������� �������� � ����������� �� ���������
		switch (state) { // �������� �������� ��������� ������
		case right: dx = speed; break;  // �������� ������
		case left: dx = -speed; break;  // �������� �����
		case jump: break;               // ������ (��� ��������� �������������� ��������)
		case stay: break;               // ��� ��������
		}

		x += dx * time; // ���������� ������� �� X � ������ ��������
		checkCollisionWithMap(dx, 0); // �������� �������� �� X

		y += dy * time; // ���������� ������� �� Y � ������ ��������
		checkCollisionWithMap(0, dy); // �������� �������� �� Y

		// �������� ������ �� ������� ������
		sf::Vector2f levelSize = level.GetLevelSize(); // ��������� �������� ������
		bool currentOutOfBounds = (x < 0 || x + w > levelSize.x || y < 0 || y + h > levelSize.y); // �������� ������ �� �������
		if (currentOutOfBounds || isOutOfBounds) { // ���� ����� ����� �� �������
			if (!isOutOfBounds) die(); // ����� ������ ������ ��� ������ ������
			if (teleportClock.getElapsedTime().asSeconds() >= 0.3f) { // �������� ������� ������������
				needRestartLevel = true; // ��������� ����� ������������� ����������� ������
				isOutOfBounds = false; // ����� ����� ������ �� �������
			}
		}
		else {
			isOutOfBounds = false; // ����� ����� ������ �� �������
		}

		// ��������� �������� �����������
		if (isResurrecting) { // �������� ��������� �����������
			if (resurrectionClock.getElapsedTime().asSeconds() >= RESURRECTION_DELAY) { // �������� ������� �����������
				isResurrecting = false; // ����� ����� �����������
				isVisible = true; // �������������� ��������� ������
			}
		}

		sprite.setPosition(x + w / 2, y + h / 2); // ���������� ������� ������� (�������������)

		if (health <= 0) { life = false; } // �������� �������� (������ ��� ������� ��������)

		if (!isMove) { speed = 0; } // ����� �������� ��� ���������� ��������

		dy = dy + 0.0015 * time; // ���������� ���������� (���������� ������������ ��������)
	}
	};

const float Player::RESURRECTION_DELAY = 0.3f; // ����� �������� ����� ������������



// ���������� ������� ��� ����������� ������������ ������
void showLoadingScreen(RenderWindow& window) {
	// ��������� ����������� ������� ������ ����
	window.setFramerateLimit(100);
	// ���������� �������� ��� ����
	Texture bgTexture;
	// �������� ������� �������� �� �����
	bgTexture.loadFromFile("images/Zaryzka.png");

	// ���� ��� ������� �������� �������� (false - �� ���������)
	bool skipRequested = false; // ���� ��� �������� ��������

	// �������� ������� ���� �� ��������
	Sprite background(bgTexture);
	// ��������� �������� ���� ��������������� ������� ����
	background.setScale(
		// ������ �������� �� ������
		static_cast<float>(window.getSize().x) / bgTexture.getSize().x,
		// ������ �������� �� ������
		static_cast<float>(window.getSize().y) / bgTexture.getSize().y
	);

	// ���������� �������� ��� ������ ��������
	Texture barTexture;
	// �������� �������� ������ �� �����
	barTexture.loadFromFile("images/polosa.png");

	// �������� ������� ������ ��������
	Sprite progressBar(barTexture);

	// ���������� ������ ��� ����� ����
	sf::SoundBuffer letterSoundBuffer;
	// �������� ����� �� �����
	letterSoundBuffer.loadFromFile("music/Bykva.wav");

	// ���������� ��������� �������
	sf::Sound letterSound;
	// �������� ������ ����� � �������
	letterSound.setBuffer(letterSoundBuffer);

	// ������ ������� ������ ��������
	const float barWidth = progressBar.getLocalBounds().width;
	// ������ ���� ����������
	const float windowWidth = window.getSize().x;
	// ������ ���� ����������
	const float windowHeight = window.getSize().y;
	// Y-���������� ��� ���������������� ������ ��������
	const float barY = (windowHeight - barTexture.getSize().y) / 1.37;

	// ��������� ��������� ������� ������ (����� ������� �������)
	progressBar.setPosition(-barWidth, barY);

	// ���������� �������� ���������
	Texture characterTexture;
	// �������� �������� ��������� �� �����
	characterTexture.loadFromFile("images/zagryzakapers.png");

	// �������� ������� ���������
	Sprite character(characterTexture);

	// ��������� �������� ���������
	// ���������� ������ � ��������
	const int characterFrames = 2;      // ���������� ������ ��������
	// ������ ������ ����� ��������
	const float frameWidth = 90.0f;     // ������ ������ �����
	// ������ ������ ����� ��������
	const float frameHeight = 122.0f;   // ������ ����� ���������� (���� 61)
	// ������� ������������ ����
	float currentFrame = 0.0f;          // ������� ����
	// �������� �������� (������� �� ����)
	const float animationSpeed = 0.1f;  // �������� ����� ������ (���/����)

	// ��������� ������� ��������� (����� ������)
	const float characterStartX = -frameWidth;
	// Y-���������� ��������� (��� ������� ��������)
	const float characterY = barY - frameHeight; // ��� ������� ��������
	// ������� X-���������� ���������
	float characterX = characterStartX;

	// ��������� �������������� �������� ��� ��������
	character.setTextureRect(IntRect(0, 0, frameWidth, frameHeight));
	// ��������� ������� ���������
	character.setPosition(characterX, characterY);

	// ���������� � ����������� ����� "�����"
	// ������ ������� ��� ���� (4 ���������� �����)
	std::vector<sf::Texture> letterTextures(4); // ���� ��������� ������������ ��� sf
	// ������ �������� ��� ����
	std::vector<sf::Sprite> letters;
	// ����� ��������� ���� (5 ����)
	std::vector<bool> letterVisible(5, false);
	// ����� ��������� ������ ����� (� ��������)
	std::vector<float> letterTimes = { 2.0f, 2.7f, 3.4f, 4.1f, 4.8f };

	// �������� ������� ��� ������ �����
	letterTextures[0].loadFromFile("images/�.png");
	letterTextures[1].loadFromFile("images/�.png");
	letterTextures[2].loadFromFile("images/�.png");
	letterTextures[3].loadFromFile("images/�.png");
	// �������� �������� ��� ����
	for (int i = 0; i < 5; i++) {
		// �������� ������� ��� ������� �����
		Sprite letter;
		// �������� ��������������� �������� ��� �����
		// ����� "�"
		if (i == 0) letter.setTexture(letterTextures[0]);      // �
		// ����� "�"
		else if (i == 1) letter.setTexture(letterTextures[1]); // �
		// ����� "�" (������)
		else if (i == 2 || i == 3) letter.setTexture(letterTextures[2]); // � (������)
		// ����� "�"
		else letter.setTexture(letterTextures[3]); // �

		// ���������� ������� � ������
		letters.push_back(letter);
	}

	// ������ ����� ������ �����
	float totalWidth = 0;
	// ������������ ������ ���� ����
	for (int i = 0; i < 5; i++) {
		totalWidth += letters[i].getLocalBounds().width;
	}
	// ���������� ����� �������
	const float spacing = 10.0f;
	// ���������� ����������� ����� �������
	totalWidth += spacing * 4;

	// ��������� ������� ��� ������������� �����
	float startX = (windowWidth - totalWidth) / 2;
	// ������� ������� ��� ����������� ����
	float currentX = startX;
	// Y-���������� ��� ���� (20% ������ ����)
	const float letterY = windowHeight * 0.2f; // 20% �� ����� ������

	// ���������� ������� ��� ������ �����
	for (int i = 0; i < 5; i++) {
		// ��������� ������� ������� �����
		letters[i].setPosition(currentX, letterY);
		// �������� ������� ��� ��������� �����
		currentX += letters[i].getLocalBounds().width + spacing;
	}

	// ������ ��� ������ ������� ��������
	Clock clock;
	// ������ ��� �������� ���������
	Clock animationClock; // ��������� ���� ��� ��������
	// ����� �������� ������ �������� (�������)
	const float barDuration = 6.0f;     // ���������� ���������� ����� � ����������
	// ����� �������� ��������� (�������)
	const float characterDuration = 6.6f; // ����� �������� ���������
	// ��������� ��� ����������� ������
	const float barTotalDistance = barWidth; // ���������� ��������� ��� ������

	// ������� ���� ���������
	while (window.isOpen()) {
		// ��������� �������
		Event event;
		// �������� ���� ������� � �������
		while (window.pollEvent(event)) {
			// �������� ����
			if (event.type == Event::Closed)
				window.close();
			// ��������� ������� �������
			if (event.type == Event::KeyPressed) {
				// ��������� ����� �������� ��������
				skipRequested = true;
			}

		}

		// ��������� ������� � ������ ��������
		float elapsed = clock.getElapsedTime().asSeconds();

		// ������ ��������� ������ �������� (0.0 - 1.0)
		float barProgress = elapsed / barDuration;
		// ����������� ��������� �������� 100%
		if (barProgress > 1.0f) barProgress = 1.0f;
		// ������ �������� ������
		float barOffset = barProgress * barTotalDistance;
		// ������ ����� ������� ������
		float newBarX = -barWidth + barOffset;
		// �������� ������ � ������� ����
		if (newBarX > 0) newBarX = 0;
		// ���������� ������� ������
		progressBar.setPosition(newBarX, barY);

		// ������ ��������� �������� ���������
		float characterProgress = elapsed / characterDuration;
		// ����������� ��������� �������� 100%
		if (characterProgress > 1.0f) characterProgress = 1.0f;
		// ������ ����� ������� ���������
		characterX = characterStartX + characterProgress * (windowWidth + frameWidth * 2);
		// ���������� ������� ���������
		character.setPosition(characterX, characterY);

		// ���������� �������� ���������
		// �������� ������������� ����� �����
		if (animationClock.getElapsedTime().asSeconds() > animationSpeed) {
			// ������� � ���������� �����
			currentFrame += 1;
			// ������������ ��������
			if (currentFrame >= characterFrames) currentFrame = 0;

			// ��������� �������������� �������� ��� �������� �����
			character.setTextureRect(IntRect(
				// X-���������� ����� � ��������
				static_cast<int>(currentFrame) * frameWidth,
				// Y-���������� ����� � ��������
				0,
				// ������ ��������� �������
				frameWidth,
				// ������ ��������� �������
				frameHeight
			));
			// ����� ������� ��������
			animationClock.restart();
		}

		// ��������� ���� �� ����������
		for (int i = 0; i < 5; i++) {
			// �������� ������� ��������� � ������� ���������
			if (!letterVisible[i] && elapsed >= letterTimes[i]) {
				// ����� ������� �����
				letterVisible[i] = true;

				// ��������������� ����� ��������� �����
				// ��������, �� ������ �� ��� ����
				if (letterSound.getStatus() != sf::Sound::Playing) {
					// ������������ �����
					letterSound.play(); // �����������, ���� ���� �� �����
				}
				else {
					// �������� ���������� ��������� �������
					sf::Sound tempSound;
					// �������� ��������� ������
					tempSound.setBuffer(letterSoundBuffer);
					// ������������ �����
					tempSound.play();
				}
			}
		}

		// ��������� �����
		// ������� ����
		window.clear();
		// ��������� ����
		window.draw(background);
		// ��������� ������ ��������
		window.draw(progressBar);
		// ��������� ��������� ������ ������
		window.draw(character); // ������ ��������� ������ ������

		// ��������� ������� ���� �����
		for (int i = 0; i < 5; i++) {
			// �������� ��������� ������� �����
			if (letterVisible[i]) {
				// ��������� �����
				window.draw(letters[i]);
			}
		}

		// ����������� ������������� �����
		window.display();

		// ������� ���������� ��������
		// �� ��������� ������� ��� ������� ��������
		if (elapsed >= characterDuration || skipRequested) {
			// ����� �� ����� ������������ ������
			break;
		}
	}
}

////////////////////////////////////////// ����� �������� ///////////////////////////////
// ����� ��� �������� � ���������� ���������
class Slider {
private:
	sf::RectangleShape track;    // ������������� - ������� ��������
	sf::CircleShape thumb;       // ���� - �������� ��������
	float minValue;              // ����������� �������� ��������
	float maxValue;              // ������������ �������� ��������
	float* value;                // ��������� �� ����������, �������� ������� ��������
	bool isDragging;             // ����: ��������������� �� �������� � ������ ������
	sf::Vector2f position;       // ������� ������ ���� �������� (X, Y)
	float width;                 // ������ ������� ��������

public:
	// ����������� ��������: ������������� ����������
	Slider(float x, float y, float w, float min, float max, float* val)
		: minValue(min), maxValue(max), value(val), isDragging(false), // ������������� ������ ������
		position(x, y), width(w) { // ������������� ������� � ������

		// ��������� �������� ���� �������
		track.setSize(sf::Vector2f(w, 10)); // ��������� �������� �������
		track.setPosition(x, y + 10); // ���������������� ������� (�������� �� Y ��� �������������)
		track.setFillColor(sf::Color(100, 100, 100)); // ���� ���������� �������
		track.setOutlineThickness(2); // ������� �������
		track.setOutlineColor(sf::Color(150, 150, 150)); // ���� �������

		// ��������� �������� ���� ��������
		thumb.setRadius(15); // ������ ��������
		thumb.setFillColor(sf::Color::Magenta); // ���� ����������
		thumb.setOutlineThickness(2); // ������� �������
		thumb.setOutlineColor(sf::Color(200, 200, 200)); // ���� �������

		// ����� ������ ��� ��������������� ���������������� ��������
		updateThumbPosition();
	}

	// ����� ��� ��������� ������� ��������
	void setPosition(float x, float y) {
		position.x = x; // ���������� X-����������
		position.y = y; // ���������� Y-����������
		track.setPosition(x, y + 10); // ����������� �������
		updateThumbPosition(); // ���������� ������� ��������
	}

	// ����� ��� ��������� ������ ��������
	void setWidth(float w) {
		width = w; // ���������� ������
		track.setSize(sf::Vector2f(w, 10)); // ��������� ������� �������
		updateThumbPosition(); // ���������� ������� ��������
	}

	// ���������� ������� �������� �� ������ �������� ��������
	void updateThumbPosition() {
		// ������������ �������� � �������� [0, 1]
		float normalizedValue = (*value - minValue) / (maxValue - minValue);
		// ������ X-���������� ������ ��������
		float thumbX = position.x + normalizedValue * width - thumb.getRadius();
		// ��������� ������� ��������
		thumb.setPosition(thumbX, position.y);
	}

	// ��������� ������� ���� ��� ��������
	void handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
		// ��������� ������� ������ ����
		if (event.type == sf::Event::MouseButtonPressed) {
			if (event.mouseButton.button == sf::Mouse::Left) {
				// �������������� ��������� �������
				sf::Vector2f mousePos = window.mapPixelToCoords(
					sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

				// �������� ����� �� ��������
				if (thumb.getGlobalBounds().contains(mousePos)) {
					isDragging = true; // ������ ��������������
				}
				// �������� ����� �� �������
				else if (track.getGlobalBounds().contains(mousePos)) {
					// ������ ������ �������� �� ������� �����
					float relativeX = mousePos.x - position.x;
					*value = minValue + (relativeX / width) * (maxValue - minValue);
					// ���������� ������� ��������
					updateThumbPosition();
				}
			}
		}
		// ��������� ���������� ������ ����
		else if (event.type == sf::Event::MouseButtonReleased) {
			if (event.mouseButton.button == sf::Mouse::Left) {
				isDragging = false; // ����� ��������������
			}
		}
		// ��������� �������� ���� ��� ��������������
		else if (event.type == sf::Event::MouseMoved && isDragging) {
			// �������������� ��������� �������
			sf::Vector2f mousePos = window.mapPixelToCoords(
				sf::Vector2i(event.mouseMove.x, event.mouseMove.y));

			// ����������� X-���������� � �������� �������
			float newX = std::max(position.x, std::min(mousePos.x, position.x + width));
			// ������ ������ ��������
			float normalizedValue = (newX - position.x) / width;
			*value = minValue + normalizedValue * (maxValue - minValue);

			// ���������� ������� ��������
			updateThumbPosition();
		}
	}

	// ��������� �������� � ����
	void draw(sf::RenderWindow& window) {
		window.draw(track); // ��������� �������
		window.draw(thumb); // ��������� ��������
	}
};

// ���������� ���������� ��� ���������
float globalVolume = 50.0f; // ��������� ������ (�������� �� ���������)

// ������� ���������� ����
void saveGame(int level, int score, int deaths,
	const std::map<int, std::set<std::pair<float, float>>>& coins, // ��������� ��������� �����
	const std::string& skin, // ������� ����
	bool item1Purchased, bool item3Purchased, // ������ �������
	float volume) { // ��������� (��������)
	std::ofstream file("savegame.dat", std::ios::binary); // �������� ����� ��� ������
	if (file.is_open()) {
		// ������ �������� ���������� ����
		file.write(reinterpret_cast<const char*>(&level), sizeof(level));
		file.write(reinterpret_cast<const char*>(&score), sizeof(score));
		file.write(reinterpret_cast<const char*>(&deaths), sizeof(deaths));

		// ������ ������ � �������
		size_t mapSize = coins.size(); // ���������� ������� � ��������
		file.write(reinterpret_cast<const char*>(&mapSize), sizeof(mapSize));
		for (const auto& entry : coins) {
			int lvl = entry.first; // ����� ������
			file.write(reinterpret_cast<const char*>(&lvl), sizeof(lvl));

			size_t setSize = entry.second.size(); // ���������� ����� �� ������
			file.write(reinterpret_cast<const char*>(&setSize), sizeof(setSize));
			for (const auto& coord : entry.second) {
				// ������ ��������� ������
				file.write(reinterpret_cast<const char*>(&coord.first), sizeof(coord.first));
				file.write(reinterpret_cast<const char*>(&coord.second), sizeof(coord.second));
			}
		}

		// ������ ������ � �����
		size_t skinLength = skin.size(); // ����� ������ �����
		file.write(reinterpret_cast<const char*>(&skinLength), sizeof(skinLength));
		file.write(skin.c_str(), skinLength); // ������ ������

		// ������ ������� �������
		file.write(reinterpret_cast<const char*>(&item1Purchased), sizeof(item1Purchased));
		file.write(reinterpret_cast<const char*>(&item3Purchased), sizeof(item3Purchased));

		// ���������� ��������� � ����
		file.write(reinterpret_cast<const char*>(&globalVolume), sizeof(globalVolume));
		file.write(reinterpret_cast<const char*>(&soundEffectsVolume), sizeof(soundEffectsVolume));
		file.close(); // �������� �����
	}
}

// ������� �������� ����
bool loadGame(int& level, int& score, int& deaths,
	std::map<int, std::set<std::pair<float, float>>>& coins, // ��������� ��� �����
	std::string& skin, // ������ ��� �����
	bool& item1Purchased, bool& item3Purchased, // ���������� ��� ������� �������
	float& volume) { // ������ ��� ��������� (�� ������������)
	std::ifstream file("savegame.dat", std::ios::binary); // �������� ����� ��� ������
	if (!file.is_open()) return false; // �������� ���������� ��������

	// ������ �������� ����������
	file.read(reinterpret_cast<char*>(&level), sizeof(level));
	file.read(reinterpret_cast<char*>(&score), sizeof(score));
	file.read(reinterpret_cast<char*>(&deaths), sizeof(deaths));

	// ������ ������ � �������
	size_t mapSize;
	file.read(reinterpret_cast<char*>(&mapSize), sizeof(mapSize));
	for (size_t i = 0; i < mapSize; ++i) {
		int lvl; // ����� ������
		file.read(reinterpret_cast<char*>(&lvl), sizeof(lvl));

		size_t setSize; // ���������� �����
		file.read(reinterpret_cast<char*>(&setSize), sizeof(setSize));
		std::set<std::pair<float, float>> coinSet; // ��������� ���������
		for (size_t j = 0; j < setSize; ++j) {
			float x, y; // ���������� ������
			file.read(reinterpret_cast<char*>(&x), sizeof(x));
			file.read(reinterpret_cast<char*>(&y), sizeof(y));
			coinSet.insert({ x, y }); // ���������� � ���������
		}
		coins[lvl] = coinSet; // ���������� ��� ������
	}

	// ������ ������ � �����
	size_t skinLength;
	file.read(reinterpret_cast<char*>(&skinLength), sizeof(skinLength));
	char* skinBuffer = new char[skinLength + 1]; // ����� ��� ������
	file.read(skinBuffer, skinLength); // ������ ������
	skinBuffer[skinLength] = '\0'; // ���������� �����������
	skin = std::string(skinBuffer); // �������� ������
	delete[] skinBuffer; // ������������ ������

	// ������ ������� �������
	file.read(reinterpret_cast<char*>(&item1Purchased), sizeof(item1Purchased));
	file.read(reinterpret_cast<char*>(&item3Purchased), sizeof(item3Purchased));

	// �������� ��������� (���� ������ ��������)
	if (!file.eof()) {
		file.read(reinterpret_cast<char*>(&globalVolume), sizeof(globalVolume));
		file.read(reinterpret_cast<char*>(&soundEffectsVolume), sizeof(soundEffectsVolume));
	}
	else {
		soundEffectsVolume = 50.0f; // �������� �� ��������� ��� ���������� ������
	}
	return true; // �������� ��������
}

// ���������� ������� ����
int menu(RenderWindow& window, bool hasSave, int& currentScore, int& savedLevel,
	std::map<int, std::set<std::pair<float, float>>>& collectedCoinsMap,
	std::string& currentSkin,
	bool& item1Purchased, bool& item3Purchased) {

	// ���������� ������ ��� ����� ���������
	SoundBuffer hoverBuffer;
	// ���������� ��������� �������
	Sound hoverSound;
	// ������� ��������� �������� ����
	if (!hoverBuffer.loadFromFile("music/button.ogg")) {
		// ����� ������ ��� ��������� ��������
		std::cerr << "Failed to load button sound!" << std::endl;
	}
	else {
		// ��������� ������ ��� �����
		hoverSound.setBuffer(hoverBuffer);
		// ��������� ��������� �����
		hoverSound.setVolume(30.f); // ��������� ���������� ���������
	}

	// ���������� ������ ��� ������
	Font font;
	// ������� �������� ������ �� �����
	if (!font.loadFromFile("Text.ttf")) {
		// ����� ������ ��� ��������� ��������
		std::cerr << "Failed to load font!" << std::endl;
		// ������� ������
		return -1;
	}

	// �������� ���������� ������� ��� ����������� �����
	Text pointsText;
	// ��������� ������ ��� ������
	pointsText.setFont(font);
	// ��������� ������� ��������
	pointsText.setCharacterSize(50);
	// ��������� ����� ������
	pointsText.setFillColor(sf::Color::White);
	// ��������� ������ � �������� ������
	pointsText.setString("POINTS: " + std::to_string(currentScore));
	// ���������������� ������
	pointsText.setPosition(1550, 20);
	// ��������� ����������� ������� ������
	window.setFramerateLimit(100);
	// ���������� ���������� ���� ����
	View initialView = window.getView();

	// ���������� ������� ��� ��������� ����
	Texture menuTexture1, menuTexture1_h, menuTexture2, menuTexture2_h,
		menuTexture4, menuTexture4_h, menuTexture3, menuTexture3_h,
		menuTexture5, menuTexture5_h, aboutTexture, aboutTextureGame, menuBackground,
		menuTexture6, menuTexture6_h, shopBackgroundTexture, installTexture, byTexture,
		menuTexture7, menuTexture7_h, settingsMenuTexture, menuTexture8, menuTexture8_h;
	// �������� �������� ��� ������ �������
	menuTexture8.loadFromFile("images/spravka.png");
	// �������� �������� ��� ������ ������� (��������� ���������)
	menuTexture8_h.loadFromFile("images/spravkah.png");

	// �������� �������� ������� ����
	menuTexture1.loadFromFile("images/StartGame.png");
	menuTexture2.loadFromFile("images/Avtor.png");
	menuTexture4.loadFromFile("images/Ob Game.png");
	menuTexture3.loadFromFile("images/Exit.png");
	menuTexture5.loadFromFile("images/Continue.png");
	menuTexture6.loadFromFile("images/shop.png");
	menuTexture7.loadFromFile("images/settings.png");

	// �������� ������� ��� ��������� ���������
	menuTexture1_h.loadFromFile("images/StartGameh.png");
	menuTexture2_h.loadFromFile("images/Avtorh.png");
	menuTexture4_h.loadFromFile("images/Ob Gameh.png");
	menuTexture3_h.loadFromFile("images/Exith.png");
	menuTexture5_h.loadFromFile("images/Continueh.png");
	menuTexture6_h.loadFromFile("images/shoph.png");
	menuTexture7_h.loadFromFile("images/settingsh.png");

	// �������� ��������������� �������
	aboutTexture.loadFromFile("images/about.png");
	aboutTextureGame.loadFromFile("images/aboutGame.png");
	menuBackground.loadFromFile("images/menu.png");
	shopBackgroundTexture.loadFromFile("images/skins.png");
	installTexture.loadFromFile("images/installed.png");
	byTexture.loadFromFile("images/by.png");
	settingsMenuTexture.loadFromFile("images/settingsmenu.png");

	// �������� �������� ��� ������
	Sprite menu1(menuTexture1), menu2(menuTexture2), menu4(menuTexture4),
		menu3(menuTexture3), menu5(menuTexture5), menuBg(menuBackground),
		menu6(menuTexture6), installBtn(installTexture), byBtn(byTexture),
		menu7(menuTexture7), menu8(menuTexture8);

	// ��������� �������� ������� ����
	Vector2u windowSize = window.getSize();
	// ����� ��� ����������� �������
	bool showAbout = false, showAboutGame = false, showShop = false, showSettings = false;

	// ������-������� ��� ���������� ������� ������
	auto updateButtonPositions = [&]() {
		// ���������� ������� ����
		windowSize = window.getSize();
		// ������ ������������ ���������������
		float scale = windowSize.y / 768.0f;

		// ��������� �������� ��� ������
		menu1.setScale(scale, scale);
		menu2.setScale(scale, scale);
		menu4.setScale(scale, scale);
		menu3.setScale(scale, scale);
		menu5.setScale(scale, scale);
		menu6.setScale(scale, scale);
		menu7.setScale(scale, scale);

		// �������� ������ ������
		std::vector<Sprite*> buttons;
		// ���������� ������ ����������� ��� ������� ����������
		if (hasSave) buttons.push_back(&menu5);
		// ���������� ��������� ������
		buttons.push_back(&menu1);
		buttons.push_back(&menu2);
		buttons.push_back(&menu4);
		buttons.push_back(&menu6);
		buttons.push_back(&menu7);
		buttons.push_back(&menu3);
		buttons.push_back(&menu8);
		// ������ ����� ������ ������
		float totalHeight = 0;
		for (auto& btn : buttons) {
			totalHeight += btn->getGlobalBounds().height - 1 * scale;
		}
		totalHeight -= 70 * scale;

		// ������ ��������� ������� �� Y
		float startY = (windowSize.y - totalHeight) / 2;
		float yPos = startY;
		// ���������������� ������
		for (auto& btn : buttons) {
			btn->setPosition(
				(windowSize.x - btn->getGlobalBounds().width) / 2,
				yPos
			);
			yPos += btn->getGlobalBounds().height + 10 * scale;
		}

		// ���������������� ������ �������
		float settingsY = menu7.getPosition().y;
		float helpX = windowSize.x - menu8.getGlobalBounds().width - 50 * scale; // ������ � ��������
		menu8.setPosition(helpX, settingsY); // �� ��� �� ������, ��� � ������ ��������
		};

	// �������������� ���������������� ������
	updateButtonPositions();

	// ���� ������ ����
	bool isMenu = true;
	// ����� ��������� ������ ����
	int menuNum = 0;
	// ��������� �� ���������� ������ ��� ��������
	Sprite* prevHoveredButton = nullptr; // ������ ���������� ������
	// ���� ��� ������ ��������� ����� �������
	bool wasInSubMenu = false; // ��� ������ ��������� ����� �������

	// ������� ���� ����
	while (isMenu) {
		Event event;
		// ��������� ������� � �������
		while (window.pollEvent(event)) {
			// ��������� ��������� ������� ����
			if (event.type == Event::Resized) {
				// ���������� ������� ���������
				FloatRect visibleArea(0, 0, event.size.width, event.size.height);
				window.setView(View(visibleArea));
				// ���������� ������� ������
				updateButtonPositions();
			}
			// ��������� �������� ����
			if (event.type == Event::Closed) {
				window.close();
				return 0;
			}
		}

		// ��������� �������������� �������
		if (showAbout || showAboutGame || showShop || showSettings) {
			// ����� �� ������� �� ESC
			if (Keyboard::isKeyPressed(Keyboard::Escape)) {
				showAbout = showAboutGame = showShop = showSettings = false;
			}

			// ��������� ������ ��������
			if (showShop) {
				// �������� ���� ��������
				Sprite shopBg(shopBackgroundTexture);
				// ������ �������� �� X
				float scaleX = static_cast<float>(windowSize.x) / shopBackgroundTexture.getSize().x;
				// ������ �������� �� Y
				float scaleY = static_cast<float>(windowSize.y) / shopBackgroundTexture.getSize().y;
				// ��������� ��������
				shopBg.setScale(scaleX, scaleY);

				// ���������� ������ � ������
				pointsText.setString("POINTS: " + std::to_string(currentScore));
				// ���������������� ������
				pointsText.setPosition(1550 * scaleX, 20 * scaleY);

				// �������� ������� ��� ������ ��������
				Texture byTexture, installTexture, installedTexture;
				byTexture.loadFromFile("images/by.png");
				installTexture.loadFromFile("images/install.png");
				installedTexture.loadFromFile("images/installed.png");

				// �������� ������������� ������
				bool item1Installed = (currentSkin == "pers2.png");
				bool item2Installed = (currentSkin == "pers.png");
				bool item3Installed = (currentSkin == "pers3.png");
				// �������������, ��� ������ ������� ������ �� ���������
				bool item2Purchased = true;

				// �������� ������ ��� ���������
				Sprite item1Btn, item2Btn, item3Btn;
				// ��������������� ������
				item1Btn.setScale(scaleX, scaleY);
				item2Btn.setScale(scaleX, scaleY);
				item3Btn.setScale(scaleX, scaleY);

				// ���������������� ������
				item1Btn.setPosition(167 * scaleX, 503 * scaleY);
				item2Btn.setPosition(468 * scaleX, 503 * scaleY);
				item3Btn.setPosition(763 * scaleX, 503 * scaleY);

				// ��������� ������� � ����������� �� ���������
				item1Btn.setTexture(item1Purchased ?
					(item1Installed ? installedTexture : installTexture) : byTexture);
				item2Btn.setTexture(item2Installed ? installedTexture : installTexture);
				item3Btn.setTexture(item3Purchased ?
					(item3Installed ? installedTexture : installTexture) : byTexture);

				// ���������� ��� ��������� ������
				static bool prevMouseLeftPressed = false;
				bool currentMouseLeftPressed = Mouse::isButtonPressed(Mouse::Left);
				Vector2i mousePos = Mouse::getPosition(window);

				// ��������� ������ �� �������
				if (currentMouseLeftPressed && !prevMouseLeftPressed) {
					// ���� �� ������� ��������
					if (item1Btn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						// ������� ��������
						if (!item1Purchased && currentScore >= 150) {
							item1Purchased = true;
							currentScore -= 150;
						}
						// ��������� �����
						else if (item1Purchased && !item1Installed) {
							currentSkin = "pers2.png";
						}
					}
					// ���� �� ������� ��������
					else if (item2Btn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						if (item2Purchased && !item2Installed) {
							currentSkin = "pers.png";
						}
					}
					// ���� �� �������� ��������
					else if (item3Btn.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
						if (!item3Purchased && currentScore >= 170) {
							item3Purchased = true;
							currentScore -= 170;
						}
						else if (item3Purchased && !item3Installed) {
							currentSkin = "pers3.png";
						}
					}
					// ���������� ��������� ����
					saveGame(savedLevel, savedScore, Player::deathCount,
						collectedCoinsMap, currentSkin, item1Purchased, item3Purchased, globalVolume);
				}
				prevMouseLeftPressed = currentMouseLeftPressed;

				// ��������� ��������
				window.clear();
				window.draw(shopBg);
				window.draw(pointsText);
				window.draw(item1Btn);
				window.draw(item2Btn);
				window.draw(item3Btn);
				window.display();
				continue;
			}
			// ��������� ������ "�� ������"
			else if (showAbout) {
				// �������� �������
				Sprite aboutSprite(aboutTexture);
				// ��������������� ��� ����
				aboutSprite.setScale(
					static_cast<float>(windowSize.x) / aboutTexture.getSize().x,
					static_cast<float>(windowSize.y) / aboutTexture.getSize().y
				);
				// ���������
				window.clear();
				window.draw(aboutSprite);
				window.display();
				continue;
			}
			// ��������� ������ "�� ����"
			else if (showAboutGame) {
				// �������� �������
				Sprite aboutGameSprite(aboutTextureGame);
				// ��������������� ��� ����
				aboutGameSprite.setScale(
					static_cast<float>(windowSize.x) / aboutTextureGame.getSize().x,
					static_cast<float>(windowSize.y) / aboutTextureGame.getSize().y
				);
				// ���������
				window.clear();
				window.draw(aboutGameSprite);
				window.display();
				continue;
			}
			// ��������� ������ ��������
			if (showSettings) {
				// �������� ���� ��������
				Sprite settingsMenuSprite(settingsMenuTexture);
				// ������ �������� �� X
				float scaleX = static_cast<float>(windowSize.x) / settingsMenuTexture.getSize().x;
				// ������ �������� �� Y
				float scaleY = static_cast<float>(windowSize.y) / settingsMenuTexture.getSize().y;
				// ��������� ��������
				settingsMenuSprite.setScale(scaleX, scaleY);

				// ���������������� �������� ���������
				float sliderX = 590 * scaleX;
				float sliderY = 459 * scaleY;
				float sliderWidth = 300 * scaleX;

				// �������� �������� ���������
				Slider volumeSlider(sliderX, sliderY, sliderWidth, 0, 100, &globalVolume);
				volumeSlider.updateThumbPosition();

				// ��������� ������ ��� ���������
				Text volumeText("", font, 30);
				volumeText.setPosition(350 * scaleX, 390 * scaleY);
				volumeText.setFillColor(sf::Color::White);

				// ����� �������� ���������
				Text volumeValueText;
				volumeValueText.setFont(font);
				volumeValueText.setCharacterSize(30);
				volumeValueText.setFillColor(sf::Color::White);
				volumeValueText.setPosition(910 * scaleX, 455 * scaleY);
				volumeValueText.setString(std::to_string(static_cast<int>(globalVolume)) + "%");

				// �������� �������� ��� �������� ��������
				float soundSliderX = 590 * scaleX;
				float soundSliderY = 559 * scaleY; // ������� ���� ������� ��������
				float soundSliderWidth = 300 * scaleX;
				Slider soundEffectsSlider(soundSliderX, soundSliderY, soundSliderWidth, 0, 100, &soundEffectsVolume);

				// ����� ��� �������� ��������
				Text soundEffectsText("", font, 30);
				soundEffectsText.setPosition(350 * scaleX, 440 * scaleY);
				soundEffectsText.setFillColor(sf::Color::White);

				// ����� �������� ��� �������� ��������
				Text soundEffectsValueText;
				soundEffectsValueText.setFont(font);
				soundEffectsValueText.setCharacterSize(30);
				soundEffectsValueText.setFillColor(sf::Color::White);
				soundEffectsValueText.setPosition(910 * scaleX, 555 * scaleY);

				// ��������� ���� ��������
				while (showSettings) {
					Event event;
					while (window.pollEvent(event)) {
						// ��������� �������� ����
						if (event.type == Event::Closed) {
							window.close();
							return 0;
						}

						// ��������� ������� ���������
						volumeSlider.handleEvent(event, window);
						soundEffectsSlider.handleEvent(event, window);
						// ����� �� ESC
						if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape) {
							showSettings = false;
							// ���������� ��������
							saveGame(savedLevel, savedScore, Player::deathCount,
								collectedCoinsMap, currentSkin,
								item1Purchased, item3Purchased, globalVolume);
						}

						// ��������� ��������� ������� ����
						if (event.type == Event::Resized) {
							windowSize = window.getSize();
							FloatRect visibleArea(0, 0, event.size.width, event.size.height);
							window.setView(View(visibleArea));

							// �������� ��������
							scaleX = static_cast<float>(windowSize.x) / settingsMenuTexture.getSize().x;
							scaleY = static_cast<float>(windowSize.y) / settingsMenuTexture.getSize().y;
							settingsMenuSprite.setScale(scaleX, scaleY);

							// ���������� ������� ���������
							sliderX = 500 * scaleX;
							sliderY = 400 * scaleY;
							sliderWidth = 300 * scaleX;

							// ���������� ��������
							volumeSlider.setPosition(sliderX, sliderY);
							volumeSlider.setWidth(sliderWidth);
							volumeSlider.updateThumbPosition();

							// ���������� ������� ������
							volumeText.setPosition(350 * scaleX, 390 * scaleY);
							volumeValueText.setPosition(820 * scaleX, 390 * scaleY);
						}
					}

					// ���������� �������� ������
					volumeValueText.setString(std::to_string(static_cast<int>(globalVolume)) + "%");
					soundEffectsValueText.setString(std::to_string(static_cast<int>(soundEffectsVolume)) + "%");
					// ��������� ������ ��������
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

		// �������� ��������� ����
		// ����� ������� ������ �� �����������
		menu1.setTexture(menuTexture1);
		menu2.setTexture(menuTexture2);
		menu4.setTexture(menuTexture4);
		menu3.setTexture(menuTexture3);
		menu6.setTexture(menuTexture6);
		menu7.setTexture(menuTexture7);
		menu8.setTexture(menuTexture8); // ���������
		// ��� ������ ����������� (���� ���� ����������)
		if (hasSave) menu5.setTexture(menuTexture5);

		// ��������� ������� �������
		Vector2i mousePos = Mouse::getPosition(window);
		// ����� ������ ����
		menuNum = 0;
		// ������� ������ ��� ��������
		Sprite* currentHoveredButton = nullptr;

		// �������� ��������� �� ������
		int counter = 1;
		// �������� ������ Continue
		if (hasSave && menu5.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu5.setTexture(menuTexture5_h);
			menuNum = 1;
			currentHoveredButton = &menu5;
		}
		// �������� ������ New Game
		else if (menu1.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu1.setTexture(menuTexture1_h);
			menuNum = hasSave ? 2 : 1;
			currentHoveredButton = &menu1;
		}
		// �������� ������ About Author
		else if (menu2.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu2.setTexture(menuTexture2_h);
			menuNum = hasSave ? 3 : 2;
			currentHoveredButton = &menu2;
		}
		// �������� ������ About Game
		else if (menu4.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu4.setTexture(menuTexture4_h);
			menuNum = hasSave ? 4 : 3;
			currentHoveredButton = &menu4;
		}
		// �������� ������ Shop
		else if (menu6.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu6.setTexture(menuTexture6_h);
			menuNum = hasSave ? 5 : 4;
			currentHoveredButton = &menu6;
		}
		// �������� ������ Settings
		else if (menu7.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu7.setTexture(menuTexture7_h);
			menuNum = hasSave ? 6 : 5;
			currentHoveredButton = &menu7;
		}
		// �������� ������ Exit
		else if (menu3.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu3.setTexture(menuTexture3_h);
			menuNum = hasSave ? 7 : 6;
			currentHoveredButton = &menu3;
		}
		// �������� ������ Help
		else if (menu8.getGlobalBounds().contains(mousePos.x, mousePos.y)) {
			menu8.setTexture(menuTexture8_h);
			menuNum = hasSave ? 8 : 7; // ������������� ���������� ����� ��� �������
			currentHoveredButton = &menu8;
		}

		// ��������������� ����� ��� ���������
		if (!showAbout && !showAboutGame && !showShop && !showSettings) {
			// ����� ��������� ����� ������ �� �������
			if (wasInSubMenu) {
				prevHoveredButton = nullptr;
				wasInSubMenu = false;
			}

			// ��������������� ����� ��� ����� ������
			if (currentHoveredButton != nullptr && currentHoveredButton != prevHoveredButton) {
				hoverSound.play();
			}
		}
		else {
			// ��������� ����� ���������� � �������
			wasInSubMenu = true;
		}

		// ���������� ���������� ������
		prevHoveredButton = currentHoveredButton;

		static bool helpButtonPressed = false; // ���� ��������� ������ �������

		//��� ���� ����� �� ���������� �������
		if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
		
			helpButtonPressed = false;
		}
		// ��������� ������ �����
		if (Mouse::isButtonPressed(Mouse::Left)) {
			// ��������� ��� ������ � �����������
			if (hasSave) {
				switch (menuNum) {
				case 1: return 2; // Continue
				case 2: return 1; // New Game
				case 3: showAbout = true; break; // �����
				case 4: showAboutGame = true; break; // �� ����
				case 5: showShop = true; break; // �������
				case 6: showSettings = true; break; // ���������
				case 7: window.close(); return 0; // �����
				case 8: if (!helpButtonPressed) {
					system("start Ballu.chm");
					helpButtonPressed = true; // ��������� ��������� ��������
				}
					  break;
				}
			}
			// ��������� ��� ����������
			else {
				switch (menuNum) {
				case 1: return 1; // New Game
				case 2: showAbout = true; break; // �����
				case 3: showAboutGame = true; break; // �� ����
				case 4: showShop = true; break; // �������
				case 5: showSettings = true; break; // ���������
				case 6: window.close(); return 0; // �����
				case 7:  if (!helpButtonPressed) {
					system("start Ballu.chm");
					helpButtonPressed = true; // ��������� ��������� ��������
				}
					  break;
				}
			}
		}

		// ��������� ��������� ����
		// ��������������� ���� ����
		menuBg.setScale(
			static_cast<float>(windowSize.x) / menuBackground.getSize().x,
			static_cast<float>(windowSize.y) / menuBackground.getSize().y
		);

		// ������� ����
		window.clear();
		// ��������� ����
		window.draw(menuBg);
		// ��������� ������ (���� ��������)
		if (hasSave) window.draw(menu5);
		window.draw(menu1);
		window.draw(menu2);
		window.draw(menu4);
		window.draw(menu6);
		window.draw(menu7);
		window.draw(menu8); // ������ �������
		window.draw(menu3);
		// ����������� �� ������
		window.display();
	}

	// �������������� ���������� ����
	window.setView(initialView);
	// ������� �������� �� ���������
	return 0;
}

////////////////////////////////////////// ������ ///////////////////////////////
class Portal : public Entity {  // ���������� ������ Portal, ������������ Entity
private:                        // ������ ��������� ������ ������
	float targetX;              // ������� ������� �� ��� X ��� �����������
	float startX;               // ��������� ������� �� ��� X
	float activationXStart;     // ������ ���� ��������� �� X (����� �������)
	float activationXEnd;       // ����� ���� ��������� �� X (������ �������)
	float speed;                // �������� ����������� �������
	bool isActivated;           // ���� ��������� ������� (����� �� ��������)
	bool isAtTarget;            // ���� ���������� ������� �������

public:                         // ������ ��������� ������� ������
	// ����������� ������ Portal
	Portal(Image& image, String Name, Level& lvl, float X, float Y, int W, int H,
		float tX, float actXStart, float actXEnd, float spd)
		: Entity(image, Name, X, Y, W, H), // ����� ������������ �������� ������ Entity
		targetX(tX),                       // ������������� ������� ������� X
		startX(X),                         // ������������� ��������� ������� X
		activationXStart(actXStart),       // ������������� ������ ���� ���������
		activationXEnd(actXEnd),           // ������������� ����� ���� ���������
		speed(spd),                        // ������������� �������� �����������
		isActivated(false),                // ������������� ����� ��������� (false)
		isAtTarget(false)                  // ������������� ����� ���������� ���� (false)
	{
		sprite.setTextureRect(IntRect(0, 0, W, H)); // ��������� �������������� �������� �������
		sprite.setOrigin(w / 2, h / 2);             // ��������� ������ �������
	}

	// ����� �������� ��������� �������
	void checkActivation(float playerX) {
		// ��������: ���� ������ �� ����������� � ����� � ���� ���������
		if (!isActivated && playerX >= activationXStart && playerX <= activationXEnd) {
			isActivated = true; // ������������ ������
		}
	}

	// ����� ���������� ��������� ������� (��������������� ������������ ������)
	void update(float time) override {
		// ���� ������ ����������� � ��� �� ������ ����
		if (isActivated && !isAtTarget) {
			float step = speed * time; // ������ ���� ����������� �� ����� time

			// �������� ������ (���� ���� ������ �� ������)
			if (targetX > startX) {
				x += step;             // ���������� ������� X
				if (x >= targetX) {    // �������� ���������� ����
					x = targetX;       // �������� �� ������� �������
					isAtTarget = true; // ��������� ����� ����������
				}
			}
			// �������� ����� (���� ���� ����� �� ������)
			else {
				x -= step;             // ���������� ������� X
				if (x <= targetX) {    // �������� ���������� ����
					x = targetX;       // �������� �� ������� �������
					isAtTarget = true; // ��������� ����� ����������
				}
			}
		}
		sprite.setPosition(x + w / 2, y + h / 2); // ���������� ������� �������
	}

	// ����� �������� ���������� �������
	bool isPortalReady() const {
		return isAtTarget; // ���������� ��������� ���������� ����
	}

	// ����� ��������� ������ �������
	sf::Vector2f getCenter() const {
		return sf::Vector2f(x + w / 2, y + h / 2); // ���������� ��������� ������
	}
};

// ����� �������� ���������
class failure : public Entity { // ���������� ������ failure, ������������ Entity
private:                        // ������ ��������� ������ ������
	bool isActivated;           // ���� ��������� �������
	float activationX;          // X-���������� ��������� �������
	float startY;               // ��������� ������� �� Y (��� ������)
	Sound fallSound;            // �������� ������ �������

public:                         // ������ ��������� ������� ������
	// ����������� ������ failure
	failure(Image& image, String Name, Level& lvl, float X, float Y, int W, int H, float actX, SoundBuffer& fallBuffer)
		: Entity(image, Name, X, Y, W, H), // ����� ������������ �������� ������
		activationX(actX),               // ������������� X-���������� ���������
		startY(Y),                       // ������������� ��������� ������� Y
		fallSound(fallBuffer)            // ������������� ����� �������
	{
		// ���� ������������ ������
	}

	// ������ ��������� ���������
	bool isActivatedPlatform() const {
		return isActivated; // ������� ����� ���������
	}

	// ������ ���������� ���������
	float getActivationX() const {
		return activationX; // ������� X-���������� ���������
	}

	// ����� ��������� �������
	void activate() {
		if (!isActivated) {               // ���� ��������� �� ������������
			isActivated = true;            // ��������� ����� ���������
			dy = 0.42f;                   // ������� ������������ ��������
			fallSound.setVolume(soundEffectsVolume); // ��������� ��������� �����
			fallSound.play();             // ��������������� ����� �������
		}
	}

	// ����� ���������� ��������� ���������
	void update(float time) override {
		if (!isActivated) {
			// ������ �� ��������� �����������
		}
		else {
			y += dy * time; // ���������� ������� Y � ������ ��������
		}
		sprite.setPosition(x + w / 2, y + h / 2); // ���������� ������� �������
	}
};

// ����� ������� ���������� ���������
class HardPlatform : public Entity { // ���������� ������ HardPlatform
private:                            // ������ ��������� ������
	float targetX;                  // ������� ������� X
	float targetY;                  // ������� ������� Y
	// ������������ ��������� ��������
	enum MoveState { MovingX, MovingY, Done } moveState;
	bool moveOrderXY;               // ������� ��������: true = X->Y, false = Y->X
	bool isActivated;               // ���� ��������� ���������
	float activationXStart;         // ������ ���� ��������� �� X
	float activationXEnd;           // ����� ���� ��������� �� X
	float activationYStart;         // ������ ���� ��������� �� Y
	float activationYEnd;           // ����� ���� ��������� �� Y
	float speedX;                   // �������� �������� �� ��� X
	float speedY;                   // �������� �������� �� ��� Y
	Sound hardSound;                // ���� �������� ���������

public:                             // ������ ��������� �������
	// ����������� ������ HardPlatform
	HardPlatform(Image& image, String Name, Level& lvl,
		float X, float Y, int W, int H,
		float tX, float tY, bool moveXY,
		float actXStart, float actXEnd,
		float actYStart, float actYEnd,
		float spdX, float spdY,
		SoundBuffer& hardBuffer)
		: Entity(image, Name, X, Y, W, H), // ������������� �������� ������
		targetX(tX),                     // ������������� ������� X
		targetY(tY),                     // ������������� ������� Y
		moveOrderXY(moveXY),             // ������������� ������� ��������
		moveState(Done),                 // ������������� ��������� (�������� Done)
		isActivated(false),              // ������������� ����� ���������
		activationXStart(actXStart),     // ������������� ������ ��������� X
		activationXEnd(actXEnd),         // ������������� ����� ��������� X
		activationYStart(actYStart),     // ������������� ������ ��������� Y
		activationYEnd(actYEnd),         // ������������� ����� ��������� Y
		speedX(spdX),                    // ������������� �������� �� X
		speedY(spdY),                    // ������������� �������� �� Y
		hardSound(hardBuffer)            // ������������� ��������� �������
	{
		sprite.setTextureRect(IntRect(0, 0, W, H)); // ��������� �������� �������
		dx = 0;                     // ������������� �������� �� X = 0
		dy = 0;                     // ������������� �������� �� Y = 0
		hardSound.setVolume(50.f);   // ��������� ������� ��������� �����
	}

	// ����� ��������� �������� ���������
	void activate() {
		if (!isActivated) {          // ���� ��������� �� ������������
			isActivated = true;       // ��������� ����� ���������
			hardSound.setVolume(soundEffectsVolume); // ���������� ���������
			hardSound.play();         // ��������������� �����

			// ����� ����������� �������� � ����������� �� �������
			if (moveOrderXY) {        // ���� ������� X->Y
				moveState = MovingX;  // ��������� ��������� "�������� �� X"
				dx = (targetX > x) ? speedX : -speedX; // ������ ����������� �� X
				dy = 0;               // ��������� �������� �� Y
			}
			else {                    // ���� ������� Y->X
				moveState = MovingY;  // ��������� ��������� "�������� �� Y"
				dy = (targetY > y) ? speedY : -speedY; // ������ ����������� �� Y
				dx = 0;               // ��������� �������� �� X
			}
		}
	}

	// ����� ���������� ��������� ���������
	void update(float time) override {
		if (isActivated) {            // ���� ��������� ������������
			switch (moveState) {      // ����� ������ �� �������� ���������
			case MovingX:             // ���������: �������� �� X
				// �������� ����������� � ���������� ����
				if ((dx > 0 && x < targetX) || (dx < 0 && x > targetX)) {
					x += dx * time;   // ��������� ������� X
				}
				else {
					x = targetX;      // �������� �� ���� X
					dx = 0;           // ����� �������� X
					// ����� ���������� ���������
					if (moveOrderXY) {
						moveState = MovingY; // ������� � �������� �� Y
						dy = (targetY > y) ? speedY : -speedY; // ����������� �� Y
					}
					else {
						moveState = Done; // ��������� ��������
					}
				}
				break;

			case MovingY:             // ���������: �������� �� Y
				// �������� ����������� � ���������� ����
				if ((dy > 0 && y < targetY) || (dy < 0 && y > targetY)) {
					y += dy * time;   // ��������� ������� Y
				}
				else {
					y = targetY;      // �������� �� ���� Y
					dy = 0;          // ����� �������� Y
					// ����� ���������� ���������
					if (!moveOrderXY) {
						moveState = MovingX; // ������� � �������� �� X
						dx = (targetX > x) ? speedX : -speedX; // ����������� �� X
					}
					else {
						moveState = Done; // ��������� ��������
					}
				}
				break;

			case Done:                // ���������: �������� ���������
				break;                // ������� ��������
			}
		}
		sprite.setPosition(x + w / 2, y + h / 2); // ���������� ������� �������
	}

	// ������� ��� ���������
	float getActivationXStart() const {
		return activationXStart; // ������� ������ ���� X
	}
	float getActivationXEnd() const {
		return activationXEnd;   // ������� ����� ���� X
	}
	float getActivationYStart() const {
		return activationYStart; // ������� ������ ���� Y
	}
	float getActivationYEnd() const {
		return activationYEnd;   // ������� ����� ���� Y
	}

	// ����� �������� ���������� ���������
	bool isActive() const {
		return isActivated; // ������� ��������� ���������
	}
};
//////////////////////////////// ����� SPIKE /////////////////////////////
class Spike : public Entity {  // ���������� ������ Spike, ������������ �� Entity
private:  // ������ �������� ������ ������
	float startPos;        // ��������� ���������� ���� (X/Y) ����� ����������
	float targetPos;       // �������� ���������� �������� ����
	bool isMovingX;        // ���� ����������� �������� (true - ��������������, false - ������������)
	bool isActivated;      // ���� ��������� ��������� ����
	float activationXStart, activationXEnd; // ������� ���� ��������� �� ��� X
	float activationYStart, activationYEnd; // ������� ���� ��������� �� ��� Y
	float platformSpeed;   // �������� ����������� ����
	Sound spikeSound;      // �������� ������ ��� ������� ���������

public:  // ������ �������� ������� ������
	// ����������� ������ Spike � �����������
	Spike(Image& image, String Name, Level& lvl, float X, float Y,
		int W, int H, float tPos, bool moveX,
		float actXStart, float actXEnd,
		float actYStart, float actYEnd,
		float speed, SoundBuffer& spikeBuffer)
		: Entity(image, Name, X, Y, W, H), // ����� ������������ �������� ������
		startPos(moveX ? X : Y),        // ������������� ��������� ������� (X ��� Y)
		targetPos(tPos),                // ������������� ������� �������
		isMovingX(moveX),               // ��������� ����������� ��������
		isActivated(false),             // ����������� ��������� - �� �����������
		activationXStart(actXStart),    // ������������� ����� ������� ���� ��������� X
		activationXEnd(actXEnd),        // ������������� ������ ������� ���� ��������� X
		activationYStart(actYStart),    // ������������� ������� ������� ���� ��������� Y
		activationYEnd(actYEnd),        // ������������� ������ ������� ���� ��������� Y
		platformSpeed(speed),           // ��������� �������� ��������
		spikeSound(spikeBuffer)         // ������������� ����� �� ������
	{  // ������ ���� ������������
		sprite.setTextureRect(IntRect(0, 0, W, H)); // ��������� �������������� �������� �������
		dx = 0; // ������������� �������������� �������� �����
		dy = 0; // ������������� ������������ �������� �����
	}  // ����� ���� ������������

	// ����� ��������� ����� ������� ��������� �� X
	float getActivationXStart() const { return activationXStart; }
	// ����� ��������� ������ ������� ��������� �� X
	float getActivationXEnd() const { return activationXEnd; }
	// ����� ��������� ������� ������� ��������� �� Y
	float getActivationYStart() const { return activationYStart; }
	// ����� ��������� ������ ������� ��������� �� Y
	float getActivationYEnd() const { return activationYEnd; }
	// ����� �������� ���������� ����
	bool isActive() const { return isActivated; }

	// ����� ��������� �������� ����
	void activate() {
		if (!isActivated) {               // ��������, �� ����������� �� ��� ���
			isActivated = true;           // ��������� ����� ���������
			spikeSound.play();            // ��������������� ����� ���������
			spikeSound.setVolume(50.f);   // ��������� ��������� ����� (50%)

			// ����� ������� ���������� � ����������� �� �����������
			float* currentPos = isMovingX ? &x : &y;
			// ������ ����������� �������� (� ���� ��� ��)
			float speed = (targetPos > *currentPos) ? platformSpeed : -platformSpeed;

			if (isMovingX) dx = speed;    // ��������� �������� �� X ���� �������������� ��������
			else dy = speed;              // ��������� �������� �� Y ���� ������������ ��������
		}
	}

	// ����� ���������� ��������� ���� (��������������� ������������ ������)
	void update(float time) override {
		if (isActivated) {  // �������� ��������� ����
			if (isMovingX) {  // ��������� ��������������� ��������
				// �������� ������������� ���������� �������� �� X
				if ((dx > 0 && x < targetPos) || (dx < 0 && x > targetPos)) {
					x += dx * time;       // ���������� ������� �� X
				}
				else {  // ���������� ������� �������
					x = targetPos;        // �������� �� �������� �������
					dx = 0;               // ����� ��������
				}
			}
			else {  // ��������� ������������� ��������
				// �������� ������������� ���������� �������� �� Y
				if ((dy > 0 && y < targetPos) || (dy < 0 && y > targetPos)) {
					y += dy * time;       // ���������� ������� �� Y
				}
				else {  // ���������� ������� �������
					y = targetPos;        // �������� �� �������� �������
					dy = 0;               // ����� ��������
				}
			}
		}
		// ���������� ������� ������� (�������������)
		sprite.setPosition(x + w / 2, y + h / 2);
	}
};  // ����� ���������� ������ Spike

// ���������� ������ Coin (������)
class Coin : public Entity {  // ������������ �� Entity
private:  // �������� ����� ������
	float CurrentFrame; // ������� ������ ����� ��������
	float frameTime;    // ����� ����������� ������ ����� � ��������
	float timer;        // ������� ������� ��� ��������

public:  // �������� ������
	// ����������� ������ Coin
	Coin(Image& image, String Name, Level& lvl, float X, float Y, int W, int H)
		: Entity(image, Name, X, Y, W, H) {  // ����� ������������ �������� ������
		sprite.setTextureRect(IntRect(0, 0, W, H)); // ��������� ���������� �������������� ��������
		CurrentFrame = 0; // ��������� ���� ��������
		frameTime = 0.1f; // �������� ����� ������� (0.1 �������)
		timer = 0;        // ����� �������� �������
	}  // ����� ������������

	// ����� ���������� ��������� ������
	void update(float time) override {
		timer += time / 1000.0f; // �������������� ����������� � �������

		// �������� ������������� ����� �����
		if (timer >= frameTime) {
			CurrentFrame += 1; // ������� � ���������� �����
			if (CurrentFrame >= 7) CurrentFrame = 0; // ������������ �������� (0-6 �����)
			timer = 0; // ����� �������

			// ���������� �������� ������� ��� ��������
			sprite.setTextureRect(IntRect(
				32 * static_cast<int>(CurrentFrame), // ������ �������� �� X (32px �� ����)
				0,                                   // Y-���������� � ��������
				32,                                  // ������ �����
				32                                   // ������ �����
			));
		}

		// ������������� ������� �� ������� �������
		sprite.setPosition(x + w / 2, y + h / 2);
	}
};  // ����� ���������� ������ Coin

// ������� ����� �������� ������
void changeLevel(Level& lvl, int& numberLevel) {
	// �������� ������ � ����������� �� ������
	if (numberLevel == 1) lvl.LoadFromFile("map/lvl1.tmx");  // �������� 1 ������
	else if (numberLevel == 2) lvl.LoadFromFile("map/lvl2.tmx");  // �������� 2 ������
	else if (numberLevel == 3) lvl.LoadFromFile("map/lvl3.tmx");  // �������� 3 ������
	else if (numberLevel == 4) lvl.LoadFromFile("map/lvl4.tmx");  // �������� 4 ������
	else if (numberLevel == 5) lvl.LoadFromFile("map/lvl5.tmx");  // �������� 5 ������
	else if (numberLevel == 6) lvl.LoadFromFile("map/lvl6.tmx");  // �������� 6 ������
	else if (numberLevel == 7) lvl.LoadFromFile("map/lvl7.tmx");  // �������� 7 ������
	else if (numberLevel == 8) lvl.LoadFromFile("map/lvl8.tmx");  // �������� 8 ������
	else if (numberLevel == 9) lvl.LoadFromFile("map/lvl9.tmx");  // �������� 9 ������
	else if (numberLevel == 10) lvl.LoadFromFile("map/lvl10.tmx"); // �������� 10 ������
	else if (numberLevel == 11) lvl.LoadFromFile("map/lvl11.tmx"); // �������� 11 ������
	else if (numberLevel == 12) lvl.LoadFromFile("map/lvl12.tmx"); // �������� 12 ������
	else if (numberLevel == 13) lvl.LoadFromFile("map/lvl13.tmx"); // �������� 13 ������
	else if (numberLevel == 14) lvl.LoadFromFile("map/lvl14.tmx"); // �������� 14 ������
	else if (numberLevel == 15) lvl.LoadFromFile("map/lvl15.tmx"); // �������� 15 ������
	// ��������� �������� ������ �� Tiled-�����
	sf::Vector2f levelSize = lvl.GetLevelSize();
	// ��������� ���������� ������� ������ ��� ������ ������
	setLevelView(levelSize);
}  // ����� ������� changeLevel

// ���������� ������ ��� ������� ������ ����
Music gameMusic;
// ���������� ������� ������� ����
int startGame(RenderWindow& window, int& numberLevel, int& currentScore, const std::string& currentSkin) {
	// ��������� ����������� FPS ��� ������� ��������
	window.setFramerateLimit(120);

	// �������� ������� ������� ������
	if (gameMusic.getStatus() != Music::Playing) {
		// �������� ����� ������� ������
		gameMusic.openFromFile("music/na_fone.wav");
		// ��������� ������������ ������
		gameMusic.setLoop(true);
		// ��������� ��������� ������
		gameMusic.setVolume(globalVolume);
		// ������ ��������������� ������
		gameMusic.play();
	}

	// ���������� ������� ������
	sf::Font font;
	// ������� �������� ������ �� �����
	if (!font.loadFromFile("Text.ttf")) {
		// ����� ������ � ������� ��� �������
		std::cerr << "������ �������� ������ Text.ttf!" << std::endl;
		// ������� false ��� ������
		return false;
	}

	// �������� ������ ��� ����������� ������
	sf::Text levelText;
	// ��������� ������ ��� ������
	levelText.setFont(font);
	// ��������� ������� ��������
	levelText.setCharacterSize(20);
	// ��������� ����� ������
	levelText.setFillColor(sf::Color::White);
	// ��������� ������� ������
	levelText.setPosition(35, 32);

	// �������� ������ ��� �������� �������
	sf::Text deathText;
	// ��������� ������
	deathText.setFont(font);
	// ��������� ������� ��������
	deathText.setCharacterSize(20);
	// ��������� ����� ������
	deathText.setFillColor(sf::Color::White);
	// ��������� ������� � ������ ����� ������
	deathText.setPosition(view.getSize().x - 150, 30);

	// �������� ������ ��� ����������� �����
	sf::Text pointsText;
	// ��������� ������
	pointsText.setFont(font);
	// ��������� ������� ��������
	pointsText.setCharacterSize(20);
	// ��������� ����� ������
	pointsText.setFillColor(sf::Color::White);
	// ��������� �������
	pointsText.setPosition(view.getSize().x - 40, 50);

	// ����� ���������� ������� ������
	view.reset(FloatRect(0, 0, 1276, 768));
	// �������� ������� ������
	Level lvl;
	// �������� ������ ������
	changeLevel(lvl, numberLevel);

	// ���������� ������ ����� ������
	SoundBuffer jumpBuffer;
	// �������� ��������� ����� ������
	jumpBuffer.loadFromFile("music/jump.wav");
	// �������� ��������� ������� ������
	Sound jump(jumpBuffer);

	// ���������� ������ ����� ������
	SoundBuffer deadBuffer;
	// �������� ��������� ����� ������
	deadBuffer.loadFromFile("music/dead.wav");

	// ���������� ������ ����� ��������� ��������
	SoundBuffer hardBuffer;
	// �������� ��������� �����
	hardBuffer.loadFromFile("music/hard.wav");
	// �������� ��������� �������
	Sound hardSound;

	// ���������� ������ ����� ������� ���������
	SoundBuffer fallBuffer;
	// �������� ��������� �����
	fallBuffer.loadFromFile("music/fall.wav");
	// �������� ��������� �������
	Sound fallSound;

	// ���������� ������ ����� �������
	SoundBuffer coinBuffer;
	// �������� ��������� �����
	coinBuffer.loadFromFile("music/coin.wav");
	// �������� ��������� �������
	Sound coinSound;
	// �������� ������ � �����
	coinSound.setBuffer(coinBuffer);
	// ��������� ���������
	coinSound.setVolume(50.f);

	// ��������� ��������� ����� �������
	coinSound.setVolume(soundEffectsVolume);
	// ��������� ��������� ����� �������
	fallSound.setVolume(soundEffectsVolume);
	// ��������� ��������� ����� ���������
	hardSound.setVolume(soundEffectsVolume);

	// ���������� ����������� ���������
	Image heroImage;
	// �������� ����������� �����
	heroImage.loadFromFile("images/" + currentSkin);

	// ���������� ����������� �����
	Image spikeImage;
	// �������� ����������� �����
	spikeImage.loadFromFile("images/spike.png");

	// ���������� ����������� �������
	Image portalImage;
	// �������� ����������� �������
	portalImage.loadFromFile("images/portalmy2.png");

	// ���������� ����������� ������������� ���������
	Image failureImage;
	// �������� ����������� ���������
	failureImage.loadFromFile("images/failure.png");

	// ���������� ����������� �������
	Image coinImage;
	// �������� ����������� �������
	coinImage.loadFromFile("images/coin.png");
	// �������� ���� �� ��������� �����
	coinImage.createMaskFromColor(Color(255, 0, 255));

	// ���������� ������� ������
	Texture continueTexture, continueTexture_h;
	Texture menuTexture, menuTexture_h;
	Texture exitTexture, exitTexture_h;

	// �������� ������� �������� ������ �����������
	continueTexture.loadFromFile("images/Continue.png");
	// �������� �������� ������ ����������� ��� ���������
	continueTexture_h.loadFromFile("images/Continueh.png");

	// �������� ������� �������� ������ ����
	menuTexture.loadFromFile("images/Menubtn.png");
	// �������� �������� ������ ���� ��� ���������
	menuTexture_h.loadFromFile("images/Menubtnh.png");

	// �������� ������� �������� ������ ������
	exitTexture.loadFromFile("images/Exit.png");
	// �������� �������� ������ ������ ��� ���������
	exitTexture_h.loadFromFile("images/Exith.png");

	// �������� �������� ������
	Sprite continueBtn(continueTexture);
	Sprite menuBtn(menuTexture);
	Sprite exitBtn(exitTexture);

	// ��������� �������� ������ �����������
	continueBtn.setScale(0.5f, 0.5f);
	// ��������� �������� ������ ����
	menuBtn.setScale(0.5f, 0.5f);
	// ��������� �������� ������ ������
	exitBtn.setScale(0.5f, 0.5f);

	// ���� �����
	bool isPaused = false;
	// ������ ��� ���������� ����
	RectangleShape overlay;

	// ��������� ������� ������ �� ������
	Object player = lvl.GetObject("player");
	// �������� ������� ������
	Player p(heroImage, "Player1", lvl, player.rect.left, player.rect.top,
		37, 50, jumpBuffer, deadBuffer, numberLevel, currentScore);

	// ������ ������� ��������
	std::list<Entity*> entities;
	// �������� ��� ������ �� �������
	std::list<Entity*>::iterator it;
	// ������ ��������
	std::list<Entity*>::iterator it2;

	// ��������� �������� ��������
	std::vector<Object> portals = lvl.GetObjects("portal");
	// ��������� ��������
	for (auto& obj : portals) {
		// ������� ���������� �������
		float targetX = obj.properties.count("targetX") ? std::stof(obj.properties.at("targetX")) : obj.rect.left;
		float activationXStart = obj.properties.count("activationXStart") ? std::stof(obj.properties.at("activationXStart")) : obj.rect.left - 100;
		float activationXEnd = obj.properties.count("activationXEnd") ? std::stof(obj.properties.at("activationXEnd")) : obj.rect.left + 100;
		float speed = obj.properties.count("speed") ? std::stof(obj.properties.at("speed")) : 0.15f;

		// �������� � ���������� �������
		entities.push_back(new Portal(
			portalImage, "portal", lvl,
			obj.rect.left, obj.rect.top,
			obj.rect.width, obj.rect.height,
			targetX, activationXStart, activationXEnd, speed
		));
	}

	// ��������� �������� �������
	std::vector<Object> coins = lvl.GetObjects("coin");
	// ��������� �������
	for (auto& coinObj : coins) {
		// ��������� ��������� �������
		float x = coinObj.rect.left;
		float y = coinObj.rect.top;
		// �������� ����� �������
		if (collectedCoinsMap[numberLevel].find({ x, y }) == collectedCoinsMap[numberLevel].end()) {
			// �������� � ���������� �������
			entities.push_back(new Coin(coinImage, "coin", lvl, x, y, 32, 32));
		}
	}

	// ��������� �������� ������������� ��������
	std::vector<Object> failures = lvl.GetObjects("failure");
	// ��������� ��������
	for (int i = 0; i < failures.size(); i++) {
		// ��������� ������� ���������
		Object obj = failures[i];
		// ���������� ���������
		float activationX = 0.0f;
		// ������� ��������� ���������
		if (obj.properties.count("activationX")) {
			try {
				// ����������� ������ � �����
				activationX = std::stof(obj.properties["activationX"]);
			}
			catch (const std::exception& e) {
				// ��������� ������ �����������
				std::cerr << "������ ������ activationX: " << e.what() << std::endl;
			}
		}
		// �������� � ���������� ���������
		entities.push_back(new failure(failureImage, "failure", lvl,
			obj.rect.left, obj.rect.top, 64, 32, activationX, fallBuffer));
	}

	// ��������� �������� ������� ��������
	std::vector<Object> hardPlatforms = lvl.GetObjects("hard");
	// ��������� ��������
	for (auto& obj : hardPlatforms) {
		// ������� ���������� ��������
		float speed = obj.properties.count("speed") ? std::stof(obj.properties.at("speed")) : 0.1f;
		float speedX = obj.properties.count("speedX") ? std::stof(obj.properties.at("speedX")) : speed;
		float speedY = obj.properties.count("speedY") ? std::stof(obj.properties.at("speedY")) : speed;
		// ��������� ����������
		float startX = obj.rect.left;
		float startY = obj.rect.top;
		// ������� ����������
		float targetX = obj.properties.count("targetX") ? std::stof(obj.properties.at("targetX")) : startX;
		float targetY = obj.properties.count("targetY") ? std::stof(obj.properties.at("targetY")) : startY;
		// ������� ��������
		bool moveOrderXY = obj.properties.count("moveOrder") ? (obj.properties.at("moveOrder") == "XY") : true;

		// ������� ���������� ���������
		float activationXStart = obj.properties.count("activationXStart") ? std::stof(obj.properties.at("activationXStart")) : 0.0f;
		float activationXEnd = obj.properties.count("activationXEnd") ? std::stof(obj.properties.at("activationXEnd")) : FLT_MAX;
		float activationYStart = obj.properties.count("activationYStart") ? std::stof(obj.properties.at("activationYStart")) : 0.0f;
		float activationYEnd = obj.properties.count("activationYEnd") ? std::stof(obj.properties.at("activationYEnd")) : FLT_MAX;

		// �������� � ���������� ���������
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

	// ��������� �������� �����
	std::vector<Object> spikePlatforms = lvl.GetObjects("spike");
	// ��������� �����
	for (auto& obj : spikePlatforms) {
		// ���� �������� �� X
		bool moveX = true;
		// ������� ����������
		float target = obj.rect.left;
		// ����������� ����������� ��������
		if (obj.properties.count("targetY")) {
			moveX = false;
			target = std::stof(obj.properties.at("targetY"));
		}
		else if (obj.properties.count("targetX")) {
			target = std::stof(obj.properties.at("targetX"));
		}

		// ������� ���������� ���������
		float activationXStart = obj.properties.count("activationXStart") ? std::stof(obj.properties.at("activationXStart")) : 0.0f;
		float activationXEnd = obj.properties.count("activationXEnd") ? std::stof(obj.properties.at("activationXEnd")) : FLT_MAX;
		float activationYStart = obj.properties.count("activationYStart") ? std::stof(obj.properties.at("activationYStart")) : 0.0f;
		float activationYEnd = obj.properties.count("activationYEnd") ? std::stof(obj.properties.at("activationYEnd")) : FLT_MAX;
		// �������� ��������
		float speed = obj.properties.count("speed") ? std::stof(obj.properties.at("speed")) : 0.1f;

		// �������� � ���������� ����
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

	// ��������� ������� ������
	sf::Vector2f levelSize = lvl.GetLevelSize();
	// ��������� ������ ��� �������
	setLevelView(levelSize);
	// ���������� ������ � ����
	window.setView(view);

	// ������ ��� ������� ������� �����
	Clock clock;
	// ���� ��������� ��������
	bool isPlatformActivated = false;
	// ��������� ���� (0-����, 3-����)
	int gameState = 0;
	// ������� ������� ����
	while (window.isOpen()) {
		// ������ ������� �����
		float time = clock.getElapsedTime().asMicroseconds();
		// ���������� �������
		clock.restart();
		// ������������ �������
		time = time / 800;

		// ��������� �������
		Event event;
		while (window.pollEvent(event)) {
			// �������� ����
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// ��������� ����� �� ������� ESC
		static bool escKeyWasPressed = false;
		bool escKeyIsPressed = Keyboard::isKeyPressed(Keyboard::Escape);

		// ������������ ��������� �����
		if (escKeyIsPressed && !escKeyWasPressed) {
			isPaused = !isPaused;
			// ����� ������
			if (isPaused) {
				gameMusic.pause();
			}
			else {
				gameMusic.play();
			}
		}
		escKeyWasPressed = escKeyIsPressed;

		// ������ ���� (���� �� �� �����)
		if (!isPaused) {
			// �������� ��������� ��������
			for (it = entities.begin(); it != entities.end(); ++it) {
				if ((*it)->name == "portal") {
					// ���������� ���� � �������
					Portal* portal = static_cast<Portal*>(*it);
					// �������� ��������� �� ������� ������
					portal->checkActivation(p.x);

					// �������� �������� ����� ������
					if (portal->isPortalReady() && p.getRect().contains(portal->getCenter())) {
						// ���������� �����
						p.AddScore(10);
						// ���������� ������
						p.completeLevel();
						// ���������� �����
						currentScore = p.playerScore;
						// ������� ���� ���������� ������
						return 1;
					}
				}
			}

			// ���������� ��������� ��������
			for (it = entities.begin(); it != entities.end();) {
				// ��������� �������
				Entity* b = *it;
				// ���������� ���������
				b->update(time);

				// �������� "�������" ��������
				if (b->life == false) {
					// �������� �� ������
					it = entities.erase(it);
					// ������������ ������
					delete b;
				}
				else {
					it++;
				}
			}

			// ���� �������
			for (it = entities.begin(); it != entities.end();) {
				Entity* entity = *it;
				// �������� ������������
				if (entity->name == "coin" && entity->getRect().intersects(p.getRect())) {
					// ���������� �����
					p.AddScore(10);
					// ���������� � ��������� �������
					collectedCoinsMap[numberLevel].insert({ entity->x, entity->y });
					// ��������� ���������
					coinSound.setVolume(soundEffectsVolume);
					// ��������������� �����
					coinSound.play();
					// �������� �������
					it = entities.erase(it);
					delete entity;
				}
				else {
					++it;
				}
			}

			// ��������� ������������ � �����������
			for (it = entities.begin(); it != entities.end(); it++) {
				if ((*it)->name == "hard" && (*it)->getRect().intersects(p.getRect())) {
					// ������ ���������� ��������
					FloatRect platformRect = (*it)->getRect();
					float overlapLeft = p.getRect().left + p.w - platformRect.left;
					float overlapRight = platformRect.left + platformRect.width - p.getRect().left;
					float overlapTop = p.getRect().top + p.h - platformRect.top;
					float overlapBottom = platformRect.top + platformRect.height - p.getRect().top;

					// ����������� ����������� ��������
					bool fromLeft = std::abs(overlapLeft) < std::abs(overlapRight);
					bool fromTop = std::abs(overlapTop) < std::abs(overlapBottom);
					float minOverlapX = fromLeft ? overlapLeft : overlapRight;
					float minOverlapY = fromTop ? overlapTop : overlapBottom;

					// ������������� ������� ������
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

			
			// �������� ������������ � ������
			for (it = entities.begin(); it != entities.end(); it++) {
				if ((*it)->getRect().intersects(p.getRect())) {
					if ((*it)->name == "spike") {
						// ������ ������
						p.die();
					}
				}
			}
			// ���������� ��������� ������
			p.update(time);

			// ��������� ������� ��������
			for (it = entities.begin(); it != entities.end(); ++it) {
				if ((*it)->name == "hard") {
					// ���������� ����
					HardPlatform* hard = static_cast<HardPlatform*>(*it);
					// �������� ������� ���������
					if (!hard->isActive() &&
						p.x >= hard->getActivationXStart() &&
						p.x <= hard->getActivationXEnd() &&
						p.y >= hard->getActivationYStart() &&
						p.y <= hard->getActivationYEnd()) {
						// ��������� ���������
						hard->activate();
					}
				}
			}

			// ��������� �����
			for (it = entities.begin(); it != entities.end(); ++it) {
				if ((*it)->name == "spike") {
					// ���������� ����
					Spike* spike = static_cast<Spike*>(*it);
					// �������� ������� ���������
					if (!spike->isActive() &&
						p.x >= spike->getActivationXStart() &&
						p.x <= spike->getActivationXEnd() &&
						p.y >= spike->getActivationYStart() &&
						p.y <= spike->getActivationYEnd()) {
						// ��������� �����
						spike->activate();
					}
				}
			}

			// �������� ������������� �������� ������
			if (p.needRestartLevel) {
				currentScore = p.playerScore;
				// ������� ��������� ����� ������
				collectedCoinsMap[numberLevel].clear();
				return 2;
			}
		}

		// ��������� ������
		window.setView(view);
		// ������� ������ ������ ����
		window.clear(Color(77, 83, 140));
		// ��������� ������
		lvl.Draw(window);

		// ��������� ������������� ��������
		for (auto it = entities.begin(); it != entities.end(); ++it) {
			if ((*it)->name == "failure") {
				// ���������� ����
				failure* plat = static_cast<failure*>(*it);
				// �������� ��������� �� ������� ������
				if (!plat->isActivatedPlatform() && p.x >= plat->getActivationX()) {
					plat->activate();
				}
			}
		}

		// ���������� ��������� ��������
		if (p.x >= 1000.0f && !isPlatformActivated) {
			for (auto it = entities.begin(); it != entities.end(); it++) {
				if ((*it)->name == "failure") {
					static_cast<failure*>(*it)->activate();
				}
			}
			isPlatformActivated = true;
		}

		// ��������� ������� ��������
		for (it = entities.begin(); it != entities.end(); it++) {
			window.draw((*it)->sprite);
		}

		// ���������� ������ ������
		levelText.setString("Level " + std::to_string(numberLevel));
		// ���������� �������� �������
		deathText.setString("Death " + std::to_string(Player::deathCount));
		// ��������� �������� �������
		window.draw(deathText);
		// ��������� ������ ������
		window.draw(levelText);
		// ��������� �������� �������
		window.draw(deathText);

		// ������������� ������� �������
		deathText.setPosition(view.getSize().x - 160, 30);
		pointsText.setPosition(view.getSize().x - 160, 50);

		// ���������� ������ �����
		pointsText.setString("POINTS " + std::to_string(p.playerScore + p.levelScore));
		// ��������� �����
		window.draw(pointsText);

		// ��������� ������ (���� �����)
		if (p.isVisible) {
			window.draw(p.sprite);
		}

		// ��������� ���� �����
		if (isPaused) {
			// ��������� ����������
			overlay.setSize(Vector2f(view.getSize().x, view.getSize().y));
			overlay.setFillColor(Color(0, 0, 0, 150));
			overlay.setPosition(view.getCenter().x - view.getSize().x / 2, view.getCenter().y - view.getSize().y / 2);
			window.draw(overlay);

			// ���������������� ������
			continueBtn.setPosition(view.getCenter().x - 100, view.getCenter().y - 50);
			menuBtn.setPosition(view.getCenter().x - 100, view.getCenter().y);
			exitBtn.setPosition(view.getCenter().x - 100, view.getCenter().y + 50);

			// ����������� ���������� ��� ����� ���������
			static SoundBuffer hoverBuffer;
			static Sound hoverSound;
			static bool soundLoaded = false;
			static Sprite* prevHovered = nullptr;

			// �������� ����� ��������� (����������)
			if (!soundLoaded) {
				if (hoverBuffer.loadFromFile("music/button.ogg")) {
					hoverSound.setBuffer(hoverBuffer);
					hoverSound.setVolume(30.f);
					soundLoaded = true;
				}
			}

			// ��������� ������� ����
			Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));

			// ����� ������� ������
			continueBtn.setTexture(continueTexture);
			menuBtn.setTexture(menuTexture);
			exitBtn.setTexture(exitTexture);

			// ����������� ��������� �� ������
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

			// ��������������� ����� ��� ���������
			if (currentHovered != prevHovered) {
				if (currentHovered != nullptr && soundLoaded) {
					hoverSound.play();
				}
				prevHovered = currentHovered;
			}

			// ��������� ������
			window.draw(continueBtn);
			window.draw(menuBtn);
			window.draw(exitBtn);

			// ��������� ������ �� �������
			if (Mouse::isButtonPressed(Mouse::Left)) {
				if (continueBtn.getGlobalBounds().contains(mousePos)) {
					// ����������� ����
					isPaused = false;
				}
				else if (menuBtn.getGlobalBounds().contains(mousePos)) {
					// ���������� ����
					saveGame(savedLevel, savedScore, Player::deathCount,
						collectedCoinsMap, currentSkin, item1Purchased, item3Purchased, globalVolume);
					// ������� � ����
					return 3;
				}
				else if (exitBtn.getGlobalBounds().contains(mousePos)) {
					// �������� ����
					window.close();
					return 0;
				}
			}
		}
		// ���������� ������
		window.display();
	}
	// ���������� ����� ��� ������
	currentScore = p.playerScore;
	// ������� ��������� ����
	return gameState;
}

// �������� ��� ������ ������
Texture winTexture;
// ����� ��� ����� ������
SoundBuffer winSoundBuffer;
// �������� ������ ������
Sound winSound;
// ���� ��������������� ����� ������ (����� �� ���������)
bool winMusicPlayed = false;

// ������� ��������� �������� ��������
void gameRunning(RenderWindow& window, int& savedLevel, int& currentScore,
	int& savedDeaths, std::map<int, std::set<std::pair<float, float>>>& savedCoins,
	std::string& currentSkin) {
	// ���������� ������������� ���� ������
	View originalView = window.getView();
	// ���� ����������� �������� ��������
	bool running = true;
	// �������� ������� ����
	while (running) {
		// ������ ������ � ��������� ����������
		int result = startGame(window, savedLevel, currentScore, currentSkin);
		// �������������� ������������ ������
		window.setView(originalView);

		// ��������� ���������� ������
		switch (result) {
		case 1: // ������� �� ��������� �������
			savedLevel++;
			// �������� ���������� ���� (15 �������)
			if (savedLevel > 15) {
				// �������� �������� ������ ������
				if (!winTexture.loadFromFile("images/win.png")) {
					// ������ �������� ��������
					std::cerr << "Failed to load win.png!" << std::endl;
				}
				// �������� ����� ������
				if (!winSoundBuffer.loadFromFile("music/win.wav")) {
					// ������ �������� �����
					std::cerr << "Failed to load win.wav!" << std::endl;
				}

				// ��������� ����� ������
				winSound.setBuffer(winSoundBuffer);
				winSound.setLoop(false); // ����������� ���������������
				winSound.setVolume(globalVolume); // ���������� ���������
				winMusicPlayed = false; // ����� ����� ���������������

				// ��������� ������� ������
				gameMusic.stop();

				// �������� ������� ������
				Sprite winSprite(winTexture);
				// ��������������� ��� ������ ����
				winSprite.setScale(
					static_cast<float>(window.getSize().x) / winTexture.getSize().x,
					static_cast<float>(window.getSize().y) / winTexture.getSize().y
				);

				// ���� ����������� ������ ������
				bool winScreen = true;
				Clock winClock; // ������ ��� ����������
				while (winScreen && window.isOpen()) {
					Event event;
					// ��������� ������� ����
					while (window.pollEvent(event)) {
						// �������� ����
						if (event.type == Event::Closed) {
							window.close();
							winScreen = false;
						}
						// ����� �� Escape
						if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape) {
							winScreen = false;
						}
					}

					// ����������� ��������������� ����� ������
					if (!winMusicPlayed) {
						winSound.play();
						winMusicPlayed = true;
					}

					// ��������� ������ ������
					window.clear();
					window.draw(winSprite);
					window.display();
				}

				// ����� �������� ���������
				savedLevel = 1; // ������� 1
				savedScore = 0; // ������� �����
				savedDeaths = 0; // ������� �������
				Player::deathCount = 0; // ����� ������������ ��������
				collectedCoinsMap.clear(); // ������� ��������� �����
				currentSkin = "pers.png"; // ���� �� ���������
				item1Purchased = false; // ����� ������� 1
				item3Purchased = false; // ����� ������� 3

				// ���������� ����������� ���������
				saveGame(savedLevel, savedScore, savedDeaths,
					collectedCoinsMap, currentSkin,
					item1Purchased, item3Purchased, globalVolume);

				// ���������� �������� �����
				running = false;
			}
			break;
		case 2: // ���������� ������
			break;
		case 3: // ������� � ����
			savedDeaths = Player::deathCount; // ���������� �������� �������
			// ���������� ���������
			saveGame(savedLevel, currentScore, savedDeaths, savedCoins, currentSkin,
				item1Purchased, item3Purchased, globalVolume);
			running = false; // ���������� �������� �����
			break;
		default: // �����
			running = false;
		}
	}
}

// ������������� ����������� ���������� �������� �������
int Player::deathCount = 0;

// ������� ������� ���������
int main() {
	// �������� �������� ���� (��������������)
	RenderWindow window(VideoMode(1376, 768), "Game", Style::Fullscreen);
	// ����������� ������� ������
	window.setFramerateLimit(60);

	// ����� ������������ ������
	showLoadingScreen(window);

	// ��������� ���������� ��� ������ ������
	Texture winTexture;
	SoundBuffer winSoundBuffer;
	Sound winSound;
	bool winMusicPlayed = false;

	// ������������� ���������� ����������
	int savedLevel = 1; // ������� �������
	int savedScore = 0; // ��������� ����
	int savedDeaths = 0; // ���������� �������
	std::map<int, std::set<std::pair<float, float>>> savedCoins; // ��������� ������
	std::string currentSkin = "pers.png"; // ������� ���� ������
	bool item1Purchased = false; // ������ ������� 1
	bool item3Purchased = false; // ������ ������� 3

	// �������� ����������� ����
	bool hasSave = loadGame(savedLevel, savedScore, savedDeaths,
		savedCoins, currentSkin, item1Purchased, item3Purchased, globalVolume);

	// �������������� �������� �������
	Player::deathCount = savedDeaths;

	// ������� ���� ����������
	while (window.isOpen()) {
		Event event;
		// ��������� ������� ����
		while (window.pollEvent(event)) {
			// ������� �������� ����
			if (event.type == Event::Closed) {
				// ���������� ����� �������
				saveGame(savedLevel, savedScore, Player::deathCount,
					collectedCoinsMap, currentSkin, item1Purchased, item3Purchased, globalVolume);
				window.close(); // �������� ����
			}
		}

		// ����������� ���� � ��������� ����������
		int menuResult = menu(window, hasSave, savedScore, savedLevel,
			collectedCoinsMap, currentSkin, item1Purchased, item3Purchased);

		// ��������� ������ "����� ����"
		if (menuResult == 1) {
			item1Purchased = false; // ����� ������� 1
			item3Purchased = false; // ����� ������� 3
			currentSkin = "pers.png"; // ����� �����
			savedLevel = 1; // ��������� �������
			savedScore = 0; // ����� �����
			savedDeaths = 0; // ����� �������
			Player::deathCount = 0;  // ����� ��������
			collectedCoinsMap.clear(); // ������� �����
		}
		// ��������� ������ "����������"
		else if (menuResult == 2) {
			Player::deathCount = savedDeaths;  // �������������� �������
			collectedCoinsMap = savedCoins; // �������������� �����
		}

		// ������ ���� ���� �� �����
		if (menuResult != 0) {
			// ������ �������� ��������
			gameRunning(window, savedLevel, savedScore, savedDeaths,
				savedCoins, currentSkin);
			// ���������� ����� ���������� ����
			saveGame(savedLevel, savedScore, Player::deathCount,
				collectedCoinsMap, currentSkin, item1Purchased, item3Purchased, globalVolume);
			// ���������� ����������� ��������
			savedDeaths = Player::deathCount;
			hasSave = true;
		}
		else {
			window.close(); // �������� ��� ������ �� ����
		}
	}
	return 0; // ���������� ���������
}