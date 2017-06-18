#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// Update these with values suitable for your hardware/network.
byte mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 16);
IPAddress server(192, 168, 0, 15);


word bit_mask1[16] = { 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000 };
word wordSaidasDigitais = 0x5555;
word memoryWordSaidasDigitais;

EthernetClient ethClient;
PubSubClient client(ethClient);

long lastReconnectAttempt = 0;


String strClienteId = "IO16";
String strTopicoToIO = strClienteId + "/toIO/";
String strTopicoFromIO = strClienteId + "/fromIO/";
const int TAMANHO_BUFFER = 20;


///////////////////////////////////////////////// setup
void setup() {
	Serial.begin(57600);
	Serial.println("Booted");

	// Configura PORTL.2 como entrada e restante do PORTL como saida
	//DDRL = B11111011;

	client.setServer(server, 1883);
	client.setCallback(callback);

	Ethernet.begin(mac, ip);
	delay(1500);
	lastReconnectAttempt = 0;
}


///////////////////////////////////////////////// loop
void loop() 
{
	atualizaSaidasDigitais(wordSaidasDigitais);

	if (!client.connected()) 
	{
		long now = millis();
		if (now - lastReconnectAttempt > 5000) 
		{
			lastReconnectAttempt = now;
			// Attempt to reconnect
			if (reconnect()) 
			{
				lastReconnectAttempt = 0;
			}
		}

		// Força serem diferentes para enviar ao broker quando estabelecer a comunicação
		memoryWordSaidasDigitais = ~wordSaidasDigitais;
	}
	else 
	{
		// Client connected
		client.loop();
		publish();
	}

	//Apenas para Teste
	if (bitRead(wordSaidasDigitais, 15))
	{
		bitClear(wordSaidasDigitais, 0);
	}
}


///////////////////////////////////////////////// publish
void publish()
{
	if (memoryWordSaidasDigitais != wordSaidasDigitais)
	{
		for (int i = 0; i <= 15; i++)
		{
			if (bitRead(wordSaidasDigitais, i) != bitRead(memoryWordSaidasDigitais, i))
			{
				char topico[TAMANHO_BUFFER];
				strTopicoFromIO.toCharArray(topico, TAMANHO_BUFFER);
				sprintf(topico, "%sS%02d", topico, i);
				char payload[2];
				sprintf(payload, "%d", bitRead(wordSaidasDigitais, i));
				client.publish(topico, payload);
				Serial.print("Message sent: ");
				Serial.print(topico);
				Serial.print(" ");
				Serial.print(payload);
				Serial.println();
			}
		}
		memoryWordSaidasDigitais = wordSaidasDigitais;
	}
}


///////////////////////////////////////////////// callback
void callback(char* topic, byte* payload, unsigned int length) 
{
	// handle message arrived
	String mensagem;
	String saidaDigital;
	String valor;
	Serial.print("Message arrived: ");
	Serial.print(topic);
	Serial.print(" ");
	for (int i = 0;i<length;i++) 
	{
		Serial.print((char)payload[i]);
		mensagem += (char)payload[i];
	}
	Serial.println();

	String topico="";
	for (int i = 0;i<TAMANHO_BUFFER;i++) 
	{
		if (topic[i]==0)
		{
			break;
		}
		topico += (char)topic[i];
	}

	//Serial.println(topico.length());
	//Serial.println(topico);
	//Serial.println(topico.substring(0, strTopicoToIO.length()+1) + " | " + strTopicoToIO + "S");
	if (topico.substring(0, strTopicoToIO.length() + 1) == (strTopicoToIO + "S"))
	{
		saidaDigital = topico.substring(strTopicoToIO.length() + 1, strTopicoToIO.length() + 3);
		//Serial.println(saidaDigital);
		if (length == 1 && saidaDigital.toInt() >= 0 && saidaDigital.toInt() <= 15)
		{
			valor = mensagem;
			if (valor == "0")
			{
				bitClear(wordSaidasDigitais, saidaDigital.toInt());
			}
			else if (valor == "1")
			{
				bitSet(wordSaidasDigitais, saidaDigital.toInt());
			}
			else bitWrite(wordSaidasDigitais, saidaDigital.toInt(), !bitRead(wordSaidasDigitais, saidaDigital.toInt()));
		}
	}
}


///////////////////////////////////////////////// reconnect
boolean reconnect() {
	char charClienteId[TAMANHO_BUFFER];
	char charTopicoToIO[TAMANHO_BUFFER];
	int i = strClienteId.length();
	strClienteId.toCharArray(charClienteId, TAMANHO_BUFFER);
	if (client.connect(charClienteId)) {
		// Once connected, publish an announcement...
		
		client.publish(charClienteId, "Connected");
		Serial.println("Connected");
		// ... and resubscribe
		String topico = strTopicoToIO + "#";
		topico.toCharArray(charTopicoToIO, TAMANHO_BUFFER);
		client.subscribe(charTopicoToIO);
	}
	return client.connected();
}


///////////////////////////////////////////////// atualizaSaidasDigitais
void atualizaSaidasDigitais(word wordSaidas)
{
	// Q1 = PORTL.3 = 46
	// Q2 = PORTL.4 = 45
	// Q3 = PORTL.5 = 44

	// Desliga Clock e Enable
	//bitWrite(PORTL, 3, 0);
	//bitWrite(PORTL, 4, 0);
	//for (int i = 15; i >= 0; i--)
	//{
	//	// Atualiza Out
	//	bitWrite(PORTL, 5, (wordSaidas & bit_mask1[i]));

	//	// Liga Clock
	//	bitWrite(PORTL, 4, 1);

	//	// Desliga Clock
	//	bitWrite(PORTL, 4, 0);

	//}

	//// Liga Enable
	//bitWrite(PORTL, 3, 1);

}