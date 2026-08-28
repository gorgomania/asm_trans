#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//Константа, определяющая количество поддерживаемых команд
const int comands_num = 4;
//Константа, определяющая количество поддерживаемых директив
const int directives_num = 7;
//Константа, определяющая поддерживаемые 16битные регистры
const char* reg_16bit = "AXCXDXBXSPBPSIDI";
//Константа, определяющая поддерживаемые 8битные регистры
const char* reg_8bit = "ALCLDLBLAHCHDHBH";
//Таблица имён (бинарное дерево)
struct names_tree {
	char* name;
	int address = 0, op_bit = 0;
	names_tree* left, * right;
};
//Таблица объектного кода (однонаправленный список)
struct objects_list {
	char* input_line, * output_line, * variable, *label;
	int offset = 0, position = 0;
	objects_list* next;
};
//Структура, возвращаемая функциями определения объектного кода команд
struct comand_prop{
	char* output_line, * variable;
};
//Структура, возвращаемая функцией определения объектного кода индекса
struct index_prop {
	int index = 0x0;
	char * variable;
};
//Функция создания элемента списка объектного кода
objects_list* create_object(char* input_line, char* output_line, char* variable, char * label, int position) {
	objects_list* new_object = NULL;
	new_object = (objects_list*)malloc(sizeof(objects_list));
	if (new_object) {
		new_object->input_line = input_line;
		new_object->output_line = output_line;
		new_object->variable = variable;
		new_object->label = label;
		new_object->position = position;
		new_object->next = NULL;
		return new_object;
	}
	return NULL;
}
//Функция определения номера команды
int define_comand(char* input_line, int position, char** table) {
	int number = 0;
	if (input_line) {
		for (number; number < comands_num; number++) {
			int i = position;
			while (input_line[i] != ' ' && input_line[i] != '\0' && input_line[i] == table[number][i - position])
				i++;
			if (i - position == (int)strlen(table[number]) && (input_line[i] == ' ' || input_line[i] == '\0'))
				return number;
		}
	}
	return -1;
}
//Функция определения номера регистра, в случае неудачного определения возвращает значение -1
int define_reg(char* input_line, const char* reg_line, int input_pos) {
	int reg_pos;
	for (int reg_num = 0; reg_num < 8; reg_num++) {
		reg_pos = reg_num * 2;
		if (input_line[input_pos] == reg_line[reg_pos] && input_line[input_pos + 1] == reg_line[reg_pos + 1])
			return reg_num;
	}
	return -1;
}
//Функция поиска переменной в таблице имён, возвращает указатель на узел дерева
names_tree* find_name(names_tree* table, char * var) {
	if (var) {
		while (table) {
			if (strcmp(var, table->name) == 0)
				return table;
			else if (strcmp(var, table->name) < 0)
				table = table->left;
			else
				table = table->right;
		}
	}
	return NULL;
}
//Функция добавления переменной в таблицу имён, вернёт NULL если такое имя уже имеется
names_tree* add_name(names_tree* top, char* var, int op_bit, char** comands_table, int& code) {
	int i = 0, name_length = 0, reg_num = -1;
	names_tree* temp, * new_variable;
	//Проверка имени на совпадение с названием команд
	int comand_num = define_comand(var, 0, comands_table);
	//Проверка имени на совпадение с названием регистра
	if (strlen(var) == 2) {
		if (var[1] == 'L' || var[1] == 'H')
			reg_num = define_reg(var, reg_8bit, 0);
		else
			reg_num = define_reg(var, reg_16bit, 0);
	}
	//Название переменной определено как название регистра
	if (reg_num != -1)
		code = 3;
	//Название  переменной определено, как название команды
	else if (comand_num != -1)
		code = 2;
	else {
		//Подготовка элемента
		new_variable = (names_tree*)malloc(sizeof(names_tree));
		if (new_variable) {
			new_variable->name = var;
			new_variable->op_bit = op_bit;
			new_variable->left = NULL;
			new_variable->right = NULL;
			//Таблица имён пустая
			if (top == NULL) {
				top = new_variable;
			}
			else {
				temp = top;
				while (true) {
					if (strcmp(new_variable->name, temp->name) == 0) {
						code = 1;
						break;
					}
					else if (strcmp(new_variable->name, temp->name) < 0) {
						if (temp->left == NULL) {
							temp->left = new_variable;
							break;
						}
						temp = temp->left;
					}
					else {
						if (temp->right == NULL) {
							temp->right = new_variable;
							break;
						}
						temp = temp->right;
					}
				}
			}
		}
	}
	return top;
}
//Функция считывания строки из файла с преобразованием к верхнему регистру и устранением лишних пробелов
char* read_line(FILE* input_file) {
	char* input_line = (char*)malloc(64 * sizeof(char));
	int i = 0, begin_coma = 0;
	bool begin_spaces = true, one_space = false, colon = false;
	if (input_line) {
		do {
			if (!begin_coma)
				input_line[i] = toupper(fgetc(input_file));
			else
				input_line[i] = fgetc(input_file);
			//Попытка определения директивы DB или DW, тогда можно и даже нужен второй пробел!
			if (one_space && !begin_coma && input_line[i] == 'D') {
				i++;
				input_line[i] = toupper(fgetc(input_file));
				if (input_line[i] == 'B' || input_line[i] == 'W') {
					i++;
					input_line[i] = toupper(fgetc(input_file));
					if (input_line[i] == ' ') {
						i++;
						input_line[i] = toupper(fgetc(input_file));
					}
				}
			}
			//Пропуск впереди стоящих пробелов
			if (begin_spaces) {
				if (input_line[i] == ' ')
					continue;
				else
					begin_spaces = false;
			}
			//Если встречается пробел
			if (!begin_coma) {
				if (input_line[i] == ' ') {
					//Второй пробел не разрешён, он пропускается и пробел после двоеточия
					if (one_space || colon)
						continue;
					//Фиксация первого пробела
					else
						one_space = true;
				}
				else
					colon = false;
			}
			if (input_line[i] == ':')
				colon = true;
			//Предположение, что далее идут ковычки
			else if (input_line[i] == 39) {
				if (!begin_coma)
					begin_coma = 1;
				else if (begin_coma == 1)
					begin_coma = 0;
			}
			else if (input_line[i] == '"') {
				if (!begin_coma)
					begin_coma = 2;
				else if (begin_coma == 2)
					begin_coma = 0;
			}
			i++;
		} while (input_line[i - 1] != '\n' && !feof(input_file));
		input_line[i - 1] = '\0';
		// Strip \r for Windows CRLF files
		if (i >= 2 && input_line[i - 2] == '\r')
			input_line[i - 2] = '\0';
	}
	return input_line;
}
//Функция определяющая является ли символ признаком начала непосредственного операнда
bool is_value(char c) {
	if (c > 47 && c < 58 || c == 45 || c == 34 || c == 39)
		return true;
	return false;
}
//Функция определяющая длину команды
int len_com(int num) {
	int len = 0;
	while (num > 0) {
		num /= 256;
		len += 8;
	}
	return len;
}
//Функция определяющая значение непосредственного операнда в виде числа, код 3 в случае неверного формата операнда, код 4 в случае выхода операнда за допустимые пределы разрядной сетки
int define_value(char* str, int pos, int& op_bit, int& code, bool index = false) {
	int i, number = 0, barrier_16bit = 65536, barrier_8bit = 256, end;
	bool negative_number = false, string = false;
	//Расчёт смещения индекса
	if (index) {
		i = 0;
		while (str[i] != ']')
			i++;
		end = i - 1;
	}
	else
		end = strlen(str) - 1;
	//Отрицательное число
	if (str[pos] == '-') {
		pos++;
		negative_number = true;
	}
	//Строка символов
	else if (str[pos] == 34 || str[pos] == 39) {
		if (str[end] != str[pos]) {
			code = 3;
			return -1;
		}
		else if (end - pos > 3 || end - pos > 2 && op_bit == 8) {
			code = 4;
			return -1;
		}
		string = true;
	}
	//Строка символов
	if (string) {
		for (i = pos + 1; i < end; i++) {
			number <<= 8;
			number += str[i];
		}
	}
	//Число представленно в 16ричном формате
	else if (str[end] == 'H') {
		//Шестнадцатеричное число должно начинаться с цифры
		if (str[pos] <= 47 || str[pos] >= 58) {
			code = 3;
			return -1;
		}
		for (i = pos; i < end; i++) {
			number <<= 4;
			if (str[i] > 47 && str[i] < 58)
				number += str[i] - 48;
			else if (str[i] > 64 && str[i] < 71)
				number += str[i] - 55;
			else {
				code = 3;
				return -1;
			}
		}
	}
	//Число представленно в двоичном формате
	else if (str[end] == 'B') {
		for (i = pos; i < end; i++) {
			number <<= 1;
			if (str[i] > 47 && str[i] < 50)
				number += str[i] - 48;
			else {
				code = 3;
				return -1;
			}
		}
	}
	//Число представленно в десятичном формате
	else {
		for (i = pos; i <= end; i++) {
			number *= 10;
			if (str[i] > 47 && str[i] < 58)
				number += str[i] - 48;
			else {
				code = 3;
				return -1;
			}
		}
	}
	//Если идёт вычисление смещения индекса определяется размер операции
	if (index) {
		if (number < 128)
			op_bit = 8;
		else
			op_bit = 16;
	}
	//Число является отрицательным
	if (negative_number) {
		//Если битность операции не определена, то выполняется определение
		if (!op_bit) {
			if (number <= barrier_8bit / 2)
				op_bit = 8;
			else
				op_bit = 16;
		}
		if (op_bit == 16) {
			if (number > barrier_16bit / 2) {
				code = 4;
				return -1;
			}
			number = 0x10000 - number;
		}
		else {
			if (number > barrier_8bit / 2) {
				code = 4;
				return -1;
			}
			number = 0x100 - number;
		}
	}
	//Число является положительным
	else {
		//Если битность операции не определена, то выполняется определение
		if (!op_bit) {
			if (number <= barrier_8bit)
				op_bit = 8;
			else
				op_bit = 16;
		}
		if (op_bit == 16) {
			if (number >= barrier_16bit) {
				code = 4;
				return -1;
			}
		}
		else if (number >= barrier_8bit) {
			code = 4;
			return -1;
		}
	}
	//Разворот последовательности байтов
	if (op_bit == 16)
		number = number % 256 * 256 + number / 256;
	return number;
}
//Функция возвращающая значение байта modregr/m
int modregrm(int mod, int reg, int rm) {
	mod <<= 6;
	reg <<= 3;
	int result = mod + reg + rm;
	return result;
}
//Функция определения имени переменной
char* define_var_inline(char* line, int pos) {
	char* variable = NULL;
	int count = 0, i = 0;
	while (line[pos + count] != ' ' && line[pos + count] != ',' && line[pos + count] != '\0' && line[pos + count] != ']' && line[pos + count] != ':')
		count++;
	variable = (char*)malloc((count + 2) * sizeof(char));
	if (variable) {
		for (i; i < count; i++)
			variable[i] = line[pos + i];
		variable[i] = '\0';
	}
	return variable;
}
//Функция определяющая значение объектного кода команды (кроме 1 байта), в случае наличия операнда индекса в виде числа
index_prop* define_index(char* str, int pos, int reg, int &op_bit, names_tree* names_table, int& code) {
	index_prop* object = NULL;
	names_tree* name;
	int index = 0x0, rm = 0x0, number = 0x0, end, index_bit = 0;
	object = (index_prop*)malloc(sizeof(index_prop));
	if (object)
		object->variable = NULL;
	pos++;
	end = pos;
	while (str[end] != ']' && str[end] != '\0')
		end++;
	//Пропущена закрывающая скобка
	if (str[end] == '\0') {
		code = 11;
		return NULL;
	}
	//Неправильно задан регистр
	if (end - pos < 2) {
		code = 10;
		return NULL;
	}
	if (str[pos] == 'S' && str[pos + 1] == 'I')
		rm = 4;
	else if (str[pos] == 'D' && str[pos + 1] == 'I')
		rm = 5;
	else {
		code = 10;
		return 0;
	}
	//Есть смещение
	if (str[pos + 2] == '+' || str[pos + 2] == '-') {
		//Пропущен операнд смещения
		if (str[pos + 3] == ']') {
			code = 12;
			return 0;
		}
		if (str[pos + 2] == '-')
			pos += 2;
		else
			pos += 3;
		//Операнд смещения число
		if (is_value(str[pos])) {
			number = define_value(str, pos, index_bit, code, true);
			//Обработка ошибок определения числа
			if (code > 0)
				return 0;
			if (index_bit == 8)
				index = modregrm(1, reg, rm) << 8;
			else
				index = modregrm(2, reg, rm) << 16;
			index += number;
		}
		//Операнд смещения память
		else {
			object->variable = define_var_inline(str, pos);
			name = find_name(names_table, object->variable);
			//Если нет такого имени в таблице имён
			if (!name) {
				code = 13;
				return NULL;
			}
			//Если размер адреса не совпадает по размеру с другим операндом
			if (op_bit) {
				if (name->op_bit != op_bit) {
					code = 14;
					return NULL;
				}
			}
			else
				op_bit = name->op_bit;
			index = modregrm(2, reg, rm) << 16;
		}
	}
	//Нет смещения
	else
		index = modregrm(0, reg, rm);
	object->index = index;
	return object;
}
//Функция определения номера директивы, -1 директивы не определены, -2 директивы больше одной в строке, -3 неверный контекст употребления директивы
int define_directive(char* input_line, int pos, char** table) {
	int number, directive_num = -1, arg_pos = 0, i;
	if (input_line) {
		while (pos < 0 || input_line[pos] != '\0') {
			pos++;
			for (number = 0; number < directives_num; number++) {
				i = pos;
				while (input_line[i] != ' ' && input_line[i] != '\0' && input_line[i] == table[number][i - pos])
					i++;
				//Определена директива
				if (i - pos == (int)strlen(table[number]) && (input_line[i] == ' ' || input_line[i] == '\0')) {
					//Директива до этого не определялась
					if (directive_num == -1) {
						//Директива в положенном месте
						if (arg_pos == 0 && (number == 1 || number == 2 || number == 6) || arg_pos == 1 && number != 1 && number != 2 && number != 6) {
							directive_num = number;
							break;
						}
						else
							return -3;
					}
					else
						return -2;
				}
			}
			arg_pos++;
			pos = i;
			while (input_line[pos] != ' ' && input_line[pos] != '\0')
				pos++;
		}
	}
	return directive_num;
}
//Функция обработки команды MOV
comand_prop* comand_mov(char* input_line, int position, names_tree *names_table, int& code) {
	comand_prop* object = NULL;
	index_prop* index_object = NULL;
	names_tree* name;
	int first_reg_num = -1, reg_num = -1, i = position + 4, op_bit = 0, number;
	bool index = false;
	unsigned int comand = 0x0;
	char* output_line, str[16];
	output_line = (char*)malloc(16 * sizeof(char));
	object = (comand_prop*)malloc(sizeof(comand_prop));
	if (!object) {
		code = 32;
		return NULL;
	}
	object->variable = NULL;
	if (!output_line)
		return NULL;
	//Если первый операнд непосредственный, то возвращает код ошибки
	if (is_value(input_line[i])) {
		code = 2;
		return NULL;
	}
	//Поиск позиции запятой
	while (input_line[i] != ',' && input_line[i] != '\0')
		i++;
	//Отсутствует запятая
	if (input_line[i] == '\0') {
		code = 7;
		return NULL;
	}
	//Пропущен первый операнд перед запятой
	if (i == position + 4) {
		code = 8;
		return NULL;
	}
	//Первый операнд индекс
	if (input_line[position + 4] == '[')
		index = true;
	else {
		//Гипотеза, что первый операнд регистр
		if (i == position + 6) {
			if (input_line[position + 5] == 'L' || input_line[position + 5] == 'H') {
				first_reg_num = define_reg(input_line, reg_8bit, position + 4);
				if (first_reg_num != -1)
					op_bit = 8;
			}
			else {
				first_reg_num = define_reg(input_line, reg_16bit, position + 4);
				if (first_reg_num != -1)
					op_bit = 16;
			}
		}
		//Первый операнд память
		if (first_reg_num == -1) {
			//Определение названия переменной и её поиск в таблице имён
			object->variable = define_var_inline(input_line, position + 4);
			name = find_name(names_table, object->variable);
			//Переменная не была найдена
			if (!name) {
				code = 13;
				return NULL;
			}
			op_bit = name->op_bit;
		}
	}
	//Пропущен второй операнд
	if (input_line[i + 1] == '\0') {
		code = 6;
		return NULL;
	}
	//Второй операнд непосредственный
	if (is_value(input_line[i + 1])) {
		number = define_value(input_line, i + 1, op_bit, code);
		//Обработка ошибок определения числа
		if (code > 0)
			return NULL;
		//Первый операнд индекс
		if (index) {
			index_object = define_index(input_line, position + 4, 0, op_bit, names_table, code);
			//Обработка ошибок определения индекса
			if (code > 0)
				return NULL;
			comand = 0xC7 << len_com(index_object->index);
			comand += index_object->index;
			sprintf(str, "%X", comand);
			strcpy(output_line, str);
			comand = 0x0;
		}
		//Первый операнд переменная
		else if (first_reg_num == -1) {
			if (op_bit == 8)
				strcpy(output_line, "C6060000");
			else
				strcpy(output_line, "C7060000");
		}
		//Первый операнд регистр
		else {
			comand += 0xB0;
			comand += first_reg_num;
			if (op_bit == 16)
				comand += 0x08;
		}
		if (op_bit == 16) {
			comand <<= 16;
			comand += number;
		}
		else {
			comand <<= 8;
			comand += number;
		}
		sprintf(str, "%X", comand);
		if (first_reg_num == -1 || index)
			strcat(output_line, str);
		else
			strcpy(output_line, str);
	}
	else {
		//Гипотеза, что второй операнд регистр
		if (input_line[i + 2] != '\0' && input_line[i + 3] == '\0') {
			if (input_line[i + 2] == 'L' || input_line[i + 2] == 'H') {
				//Несовместимый размер операндов
				if (op_bit == 16) {
					code = 1;
					return NULL;
				}
				reg_num = define_reg(input_line, reg_8bit, i + 1);
				//Если первый операнд индекс, а второй регистр, то количество бит операндов устанавливается по регистру
				if (!op_bit && reg_num != -1)
					op_bit = 8;
			}
			else {
				//Несовместимый размер операндов
				if (op_bit == 8) {
					code = 1;
					return NULL;
				}
				reg_num = define_reg(input_line, reg_16bit, i + 1);
				//Если первый операнд индекс, а второй регистр, то количество бит операндов устанавливается по регистру
				if (!op_bit && reg_num != -1)
					op_bit = 16;
			}
		}
		//Первый и второй операнды регистры
		if (first_reg_num != -1 && reg_num != -1) {
			if (op_bit == 16)
				comand = 0x8B00 + modregrm(3, first_reg_num, reg_num);
			else
				comand = 0x8A00 + modregrm(3, first_reg_num, reg_num);
		}
		//Первый операнд память, второй регистр
		else if (first_reg_num == -1 && reg_num != -1) {
			if (index) {
				index_object = define_index(input_line, position + 4, reg_num, op_bit, names_table, code);
				//Обработка ошибок определения индекса
				if (code > 0)
					return NULL;
				if (op_bit == 16)
					comand = 0x89;
				else
					comand = 0x88;
				comand <<= len_com(index_object->index);
				comand += index_object->index;
			}
			//Второй операнд аккумулятор
			else if (reg_num == 0) {
				if (op_bit == 16)
					comand = 0xA30000;
				else
					comand = 0xA20000;
			}
			//Второй операнд регистр
			else {
				if (op_bit == 16)
					comand = 0x89;
				else
					comand = 0x88;
				comand <<= 8;
				comand += modregrm(0, reg_num, 6);
				comand <<= 16;
			}
		}
		//Второй операнд память
		else {
			//Первый операнд память
			if (first_reg_num == -1) {
				code = 5;
				return NULL;
			}
			//Память является индексом
			if (input_line[i + 1] == '[') {
				index_object = define_index(input_line, i + 1, first_reg_num, op_bit, names_table, code);
				//Обработка ошибок определения индекса
				if (code > 0)
					return NULL;
				if (op_bit == 16)
					comand = 0x8B;
				else
					comand = 0x8A;
				comand <<= len_com(index_object->index);
				comand += index_object->index;
			}
			else {
				//Определение названия переменной и её поиск в таблице имён
				object->variable = define_var_inline(input_line, i + 1);
				name = find_name(names_table, object->variable);
				//Переменная не была найдена
				if (!name) {
					code = 13;
					return NULL;
				}
				//Не совпадение размеров операндов
				if (op_bit != name->op_bit) {
					code = 1;
					return NULL;
				}
				//Первый операнд, аккумулятор
				if (first_reg_num == 0) {
					if (op_bit == 16)
						comand = 0xA10000;
					else
						comand = 0xA00000;
				}
				//Первый операнд регистр
				else {
					if (op_bit == 16)
						comand += 0x8B;
					else
						comand += 0x8A;
					comand <<= 8;
					comand += modregrm(0, first_reg_num, 6);
					comand <<= 16;
				}
			}
		}
		sprintf(str, "%X", comand);
		strcpy(output_line, str);
	}
	//Дополнение объекта
	if (index_object && index_object->variable)
		object->variable = index_object->variable;
	object->output_line = output_line;
	return object;
}
//Функция обработки команды XCHG
comand_prop* comand_xchg(char* input_line, int position, names_tree* names_table, int& code) {
	comand_prop* object = NULL;
	index_prop* index_object = NULL;
	names_tree* name;
	char str[16];
	unsigned int comand = 0x0;
	int reg_num = -1, first_reg_num = -1, i = position + 5, op_bit = 0;
	bool index = false;
	object = (comand_prop*)malloc(sizeof(comand_prop));
	if (!object) {
		code = 32;
		return NULL;
	}
	object->variable = NULL;
	//Если первый операнд является непосредственным, то возвращает код ошибки
	if (is_value(input_line[i])) {
		code = 2;
		return NULL;
	}
	//Поиск позиции запятой
	while (input_line[i] != ',' && input_line[i] != '\0') {
		i++;
	}
	//Запятая отсутствует в бинарной команде
	if (input_line[i] == '\0') {
		code = 7;
		return NULL;
	}
	//Пропущен первый операнд перед запятой
	if (i == position + 5) {
		code = 8;
		return NULL;
	}
	//Первый операнд индекс
	if (input_line[position + 5] == '[')
		index = true;
	//Гипотеза, что первый операнд регистр
	//Если гипотеза не подтвердится, то первый операнд память
	else if (i == position + 7) {
		if (input_line[position + 6] == 'L' || input_line[position + 6] == 'H') {
			first_reg_num = define_reg(input_line, reg_8bit, position + 5);
			if (first_reg_num != -1)
				op_bit = 8;
		}
		else {
			first_reg_num = define_reg(input_line, reg_16bit, position + 5);
			if (first_reg_num != -1)
				op_bit = 16;
		}
	}
	//Пропущен второй операнд
	if (input_line[i + 1] == '\0') {
		code = 6;
		return NULL;
	}
	//Второй операнд непосредственный
	if (is_value(input_line[i + 1])) {
		code = 9;
		return NULL;
	}
	//Гипотеза, что второй операнд регистр
	if (input_line[i + 2] != '\0' && input_line[i + 3] == '\0') {
		if (input_line[i + 2] == 'L' || input_line[i + 2] == 'H') {
			//Несовместимый размер операндов
			if (op_bit == 16) {
				code = 1;
				return NULL;
			}
			reg_num = define_reg(input_line, reg_8bit, i + 1);
			//Если первый операнд память, а второй регистр, то количество бит операндов устанавливается по регистру
			if (!op_bit && reg_num != -1)
				op_bit = 8;
		}
		else {
			//Несовместимый размер операндов
			if (op_bit == 8) {
				code = 1;
				return NULL;
			}
			reg_num = define_reg(input_line, reg_16bit, i + 1);
			//Если первый операнд память, а второй регистр, то количество бит операндов устанавливается по регистру
			if (!op_bit && reg_num != -1)
				op_bit = 16;
		}
	}
	//Один из операндов аккумулятор второй регистр, только для 32бит
	if (first_reg_num * reg_num == 0 && first_reg_num + reg_num > 0 && op_bit == 16)
		comand = 0x90 + reg_num + first_reg_num;
	//Оба операнда регистры
	else if (first_reg_num >= 0 && reg_num >= 0) {
		comand += 0x8600 + modregrm(3, first_reg_num, reg_num);
		if (op_bit == 16)
			comand += 0x0100;
	}
	//Оба операнда память
	else if (first_reg_num == -1 && reg_num == -1) {
		code = 5;
		return NULL;
	}
	//Один операнд память другой регистр
	else {
		if (op_bit == 16)
			comand = 0x87;
		else
			comand = 0x86;
		//Память - индекс
		if (index || input_line[i + 1] == '[') {
			//Индекс первый операнд
			if (index)
				index_object = define_index(input_line, position + 5, reg_num, op_bit, names_table, code);
			//Индекс второй операнд
			else
				index_object = define_index(input_line, i + 1, first_reg_num, op_bit, names_table, code);
			//Обработка ошибок определения индекса
			if (code > 0)
				return NULL;
			comand <<= len_com(index_object->index);
			comand += index_object->index;
		}
		else {
			//Первый операнд память
			if (first_reg_num == -1) {
				comand <<= 8;
				comand += modregrm(0, reg_num, 6);
				comand <<= 16;
				//Определение названия переменной
				object->variable = define_var_inline(input_line, position + 5);
			}
			//Второй операнд память
			else {
				comand <<= 8;
				comand += modregrm(0, first_reg_num, 6);
				comand <<= 16;
				//Определение названия переменной
				object->variable = define_var_inline(input_line, i + 1);
			}
			//Поиск переменной в таблице имён
			name = find_name(names_table, object->variable);
			//Переменная не была найдена
			if (!name) {
				code = 13;
				return NULL;
			}
			//Несовпадение размеров операндов
			if (op_bit != name->op_bit) {
				code = 1;
				return NULL;
			}
		}
	}
	sprintf(str, "%X", comand);
	//Дополнение объекта
	if (index_object && index_object->variable)
		object->variable = index_object->variable;
	object->output_line = (char*)malloc((strlen(str) + 2)*sizeof(char));
	if (object->output_line)
		strcpy(object->output_line, str);
	return object;
}
//Функция обработки команды DEC
comand_prop* comand_dec(char* input_line, int position, names_tree* names_table, int& code) {
	comand_prop* object = NULL;
	index_prop* index_object = NULL;
	names_tree* name;
	char str[16];
	unsigned int comand = 0x0;
	int reg_num = -1, i = position + 4, op_bit = 0;
	object = (comand_prop*)malloc(sizeof(comand_prop));
	if (!object) {
		code = 32;
		return NULL;
	}
	object->variable = NULL;
	//Пропущен первый операнд
	if (strlen(input_line) < 5) {
		code = 8;
		return NULL;
	}
	//Если операнд является непосредственным, то возвращает код ошибки
	if (is_value(input_line[i])) {
		code = 2;
		return NULL;
	}
	//Поиск позиции запятой
	while (input_line[i] != ',' && input_line[i] != '\0') {
		i++;
	}
	//Если запятая присутствует в унарной команде
	if (input_line[i] != '\0') {
		code = 15;
		return NULL;
	}
	//Первый операнд индекс
	if (input_line[position + 4] == '[') {
		index_object = define_index(input_line, position + 4, 1, op_bit, names_table, code);
		//Обработка ошибок определения индекса
		if (code > 0)
			return NULL;
		if (op_bit == 16)
			comand = 0xFF;
		else
			comand = 0xFE;
		comand <<= len_com(index_object->index);
		comand += index_object->index;
	}
	else {
		//Гипотеза, что первый операнд регистр
		if (strlen(input_line) == (size_t)(position + 6)) {
			if (input_line[position + 5] == 'L' || input_line[position + 5] == 'H') {
				reg_num = define_reg(input_line, reg_8bit, position + 4);
				if (reg_num != -1)
					op_bit = 8;
			}
			else {
				reg_num = define_reg(input_line, reg_16bit, position + 4);
				if (reg_num != -1)
					op_bit = 16;
			}
		}
		//Операнд память
		if (reg_num == -1) {
			//Определение названия переменной
			object->variable = define_var_inline(input_line, position + 4);
			name = find_name(names_table, object->variable);
			//Переменная не была найдена
			if (!name) {
				code = 13;
				return NULL;
			}
			if (name->op_bit == 8)
				comand = 0xFE;
			else
				comand = 0xFF;
			comand <<= 8;
			comand += modregrm(0, 1, 6);
			comand <<= 16;
		}
		//Операнд регистр 32бит
		else if (op_bit == 16)
			comand = 0x48 + reg_num;
		else {
			comand = 0xFE;
			comand <<= 8;
			comand += modregrm(3, 1, reg_num);
		}
	}
	sprintf(str, "%X", comand);
	//Дополнение объекта
	if (index_object && index_object->variable)
		object->variable = index_object->variable;
	object->output_line = (char*)malloc((strlen(str) + 2) * sizeof(char));
	if (object->output_line)
		strcpy(object->output_line, str);
	return object;
}
//Функция обработки команды LOOP
comand_prop* comand_loop(char* input_line, int position, names_tree* names_table, int& code) {
	comand_prop* object = NULL;
	names_tree* name;
	char str[16];
	unsigned int comand = 0x0;
	int i = position + 5, op_bit = 0;
	object = (comand_prop*)malloc(sizeof(comand_prop));
	if (!object) {
		code = 32;
		return NULL;
	}
	object->variable = NULL;
	//Пропущен первый операнд
	if (strlen(input_line) < 6) {
		code = 8;
		return NULL;
	}
	//Если операнд является непосредственным, то возвращает код ошибки
	if (is_value(input_line[i])) {
		code = 2;
		return NULL;
	}
	//Поиск позиции запятой
	while (input_line[i] != ',' && input_line[i] != '\0') {
		i++;
	}
	//Если запятая присутствует в унарной команде
	if (input_line[i] != '\0') {
		code = 15;
		return NULL;
	}
	//Операнд память
	//Определение названия переменной
	object->variable = define_var_inline(input_line, position + 5);
	name = find_name(names_table, object->variable);
	//Переменная не была найдена
	if (!name) {
		code = 13;
		return NULL;
	}
	if (name->op_bit) {
		code = 16;
		return NULL;
	}
	comand = 0xE2;
	comand <<= 8;
	sprintf(str, "%X", comand);
	//Дополнение объекта
	object->output_line = (char*)malloc((strlen(str) + 2) * sizeof(char));
	if (object->output_line)
		strcpy(object->output_line, str);
	return object;
}
//Функция определения строки в таблице
void define_table_line(char** table, int pos, const char* line) {
	if (table[pos])
		strcpy(table[pos], line);
}
//Очистка дерева
void tree_clean(names_tree* table) {
	if (table) {
		tree_clean(table->left);
		tree_clean(table->right);
		free(table);
	}
}
//Очистка мусора
void garbage_clean(names_tree* table, objects_list* list) {
	objects_list* temp;
	tree_clean(table);
	temp = list;
	while (temp) {
		list = list->next;
		free(temp);
		temp = list;
	}
}
//Обработка кодов ошибок
void error_print(int code, int line_num, names_tree* table, objects_list* list) {
	switch (code) {
	case 0:
		printf("Код 0: Имя метки задано неправильно, строка %d.\n", line_num);
		break;
	case 1:
		printf("Код 1: Несовместимый размер операндов, строка %d.\n", line_num);
		break;
	case 2:
		printf("Код 2: Первый операнд не может быть числом, строка %d.\n", line_num);
		break;
	case 3:
		printf("Код 3: Неверный формат числа, строка %d.\n", line_num);
		break;
	case 4:
		printf("Код 4: Число выходит за пределы разрядной сетки, строка %d.\n", line_num);
		break;
	case 5:
		printf("Код 5: Недопустимый формат операции (ОП, ОП), строка %d.\n", line_num);
		break;
	case 6:
		printf("Код 6: Пропущен второй операнд, строка %d.\n", line_num);
		break;
	case 7:
		printf("Код 7: Отсутствует запятая в бинарной команде, строка %d.\n", line_num);
		break;
	case 8:
		printf("Код 8: Пропущен первый операнд, строка %d.\n", line_num);
		break;
	case 9:
		printf("Код 9: Второй операнд не может быть числом, строка %d.\n", line_num);
		break;
	case 10:
		printf("Код 10: Неправильно задан регистр индекса, строка %d.\n", line_num);
		break;
	case 11:
		printf("Код 11: Пропущена закрывающая скобка индекса, строка %d.\n", line_num);
		break;
	case 12:
		printf("Код 12: Пропущен операнд смещения, строка %d.\n", line_num);
		break;
	case 13:
		printf("Код 13: Переменная не была объявлена, строка %d.\n", line_num);
		break;
	case 14:
		printf("Код 14: Смещение индекса не совпадает по размеру с другим операндом, строка %d.\n", line_num);
		break;
	case 15:
		printf("Код 15: Унарная команда имеет 2 операнда, строка %d.\n", line_num);
		break;
	case 16:
		printf("Код 16: Операнд должен быть меткой, строка %d.\n", line_num);
		break;
	case 17:
		printf("Код 17: Имя уже используется, строка %d.\n", line_num);
		break;
	case 18:
		printf("Код 18: Имя совпадает с названием команды, строка %d.\n", line_num);
		break;
	case 19:
		printf("Код 19: Имя совпадает с названием регистра, строка %d.\n", line_num);
		break;
	case 20:
		printf("Код 20: Больше одной директивы в строке, строка %d.\n", line_num);
		break;
	case 21:
		printf("Код 21: Неверный контекст употребления директивы, строка %d.\n", line_num);
		break;
	case 22:
		printf("Код 22: После директивы ENDS должна следовать директива END, строка %d.\n", line_num);
		break;
	case 23:
		printf("Код 23: Имя сегмента указано неправильно, строка %d.\n", line_num);
		return;
	case 24:
		printf("Код 24: После команды прерывания должны следовать директивы DB, DW, ENDS, строка %d.\n", line_num);
		return;
	case 25:
		printf("Код 25: Команда прерывания употребляется только со значением 21H, строка %d.\n", line_num);
		return;
	case 26:
		printf("Код 26: После директивы ORG должен следовать блок команд, завершающийся командой INT 21H, строка %d.\n", line_num);
		break;
	case 27:
		printf("Код 27: Выражение после директивы ORG должно быть числом равным 256 или 0, строка %d.\n", line_num);
		break;
	case 28:
		printf("Код 28: Необходимо определить директиву ORG, строка %d.\n", line_num);
		break;
	case 29:
		printf("Код 29: Код должен начинаться с директивы SEGMENT, строка %d.\n", line_num);
		break;
	case 30:
		printf("Код 30: Не удалось распознать команду, строка %d.\n", line_num);
		break;
	case 31:
		printf("Код 31: Структура программы не соответствует заявленным требованиям\n");
		break;
	case 32:
		printf("Код 32: Ошибка выделения памяти, строка %d.\n", line_num);
		break;
	}
	garbage_clean(table, list);
	return;
}
//Главная функция
int main() {
	//Определение таблиц
	char** comands_table, ** directives_table, * input_line, * output_line, str[10], *variable, *label = NULL;
	names_tree* names_table = NULL, * name;
	comand_prop* comand = NULL;
	objects_list* objects_beg = NULL, * objects_end = NULL, * temp = NULL;
	int line_num = 0, offset = -1, op_bit = 16, code = 0, number, i, directive_num, count, comand_num, position, default_offset;
	bool int21h = false, ends = false, end = false;
	FILE* input_file, * listing_file, *object_file;
	//Объявление таблицы директив
	directives_table = (char**)malloc(directives_num * 16 * sizeof(char*));
	if (directives_table) {
		for (i = 0; i < directives_num; i++)
			directives_table[i] = (char*)malloc(16 * sizeof(char));
		define_table_line(directives_table, 0, "SEGMENT");
		define_table_line(directives_table, 1, "ORG");
		define_table_line(directives_table, 2, "INT");//Исключение
		define_table_line(directives_table, 3, "DB");
		define_table_line(directives_table, 4, "DW");
		define_table_line(directives_table, 5, "ENDS");
		define_table_line(directives_table, 6, "END");
	}
	//Объявление таблицы команд
	comands_table = (char**)malloc(comands_num * 16 * sizeof(char*));
	if (comands_table) {
		for (i = 0; i < comands_num; i++)
			comands_table[i] = (char*)malloc(16 * sizeof(char));
		define_table_line(comands_table, 0, "MOV");
		define_table_line(comands_table, 1, "XCHG");
		define_table_line(comands_table, 2, "DEC");
		define_table_line(comands_table, 3, "LOOP");
	}
	//Работа с данными из входного файла
	input_file = fopen("code.txt", "r");
	if (!input_file) {
		puts("Ошибка: Не удалось открыть входной файл");
		return 1;
	}
	listing_file = fopen("listing.txt", "w");
	if (!listing_file) {
		puts("Ошибка: Не удалось создать файл листинга");
		return 1;
	}
	object_file = fopen("object_code.txt", "w");
	if (!object_file) {
		puts("Ошибка: Не удалось создать файл объектного кода");
		return 1;
	}
	//Первый проход транслятора (Считывание строк, построение таблицы имён, проверка структуры программы, обработка директив)
	while (!feof(input_file) && !end) {
		input_line = read_line(input_file);
		output_line = NULL;
		variable = NULL;
		label = NULL;
		line_num++;
		position = 0;
		//Пропуск пустой строки
		if (strlen(input_line) == 0)
			continue;
		//Поиск двоеточия в строке, с целью определения метки
		i = 0;
		while (input_line[i] != ':' && input_line[i] != '\0') {
			i++;
		}
		if (input_line[i] == ':') {
			position = i + 1;
			//Определение имени метки
			label = define_var_inline(input_line, 0);
			if ((int)strlen(label) < i) {
				error_print(0, line_num, names_table, objects_beg);
				return 1;
			}
			//Добавление метки в таблицу имён и обработка возможных ошибок
			names_table = add_name(names_table, label, 0, comands_table, code);
			if (code) {
				error_print(code + 16, line_num, names_table, objects_beg);
				return 1;
			}
		}
		//Если помимо метки в строке имеются команды
		if (position != (int)strlen(input_line)) {
			//Поиск директив в строке
			directive_num = define_directive(input_line, position - 1, directives_table);
			//Обработка возможных ошибок определения директивы
			if (directive_num == -2) {
				error_print(20, line_num, names_table, objects_beg);
				return 1;
			}
			if (directive_num == -3) {
				error_print(21, line_num, names_table, objects_beg);
				return 1;
			}
			//Директива ends уже определена
			if (ends) {
				//Фиксируется конец программы
				if (directive_num == 6)
					end = true;
				else {
					error_print(22, line_num, names_table, objects_beg);
					return 1;
				}
			}
			//Директива int 21h уже определена
			else if (int21h) {
				//Директива определения данных
				if (directive_num == 3 || directive_num == 4) {
					op_bit = 8 * (directive_num - 2);
					//Определение имени переменной
					variable = define_var_inline(input_line, position);
					//Добавление переменной в таблицу имён и обработка возможных ошибок
					names_table = add_name(names_table, variable, op_bit, comands_table, code);
					if (code) {
						error_print(code + 16, line_num, names_table, objects_beg);
						return 1;
					}
					//Поиск второго пробела в исходной строке
					i = position;
					count = 2;
					while (count) {
						i++;
						if (input_line[i] == ' ')
							count--;
					}
					//Определение значения переменной и обработка ошибок
					number = define_value(input_line, i + 1, op_bit, code);
					if (code) {
						error_print(code, line_num, names_table, objects_beg);
						return 1;
					}
					//Добавление строки в листинг
					sprintf(str, "%X", number);
					//Добавление ведущих нулей если их не хватает
					while (strlen(str) < (size_t)(op_bit / 4)) {
						int length = strlen(str);
						i = length;
						while (i > 0) {
							str[i] = str[i - 1];
							i--;
						}
						str[i] = '0';
						str[length + 1] = '\0';
					}
					output_line = (char*)malloc(strlen(str) + 2);
					if (output_line)
						strcpy(output_line, str);
				}
				//Определена директива ENDS, проверка имени сегмента
				else if (directive_num == 5) {
					variable = define_var_inline(input_line, position);
					if (strcmp(variable, names_table->name)) {
						error_print(23, line_num, names_table, objects_beg);
						return 1;
					}
					ends = true;
				}
				else {
					error_print(24, line_num, names_table, objects_beg);
					return 1;
				}
			}
			//Имя сегмента уже определено
			else if (names_table) {
				//Директива org уже определена
				if (offset >= 0) {
					//После директивы org была определена директива
					if (directive_num >= 0) {
						//Директива int 21h
						if (directive_num == 2) {
							number = define_value(input_line, position + 4, op_bit, code);
							if (number != 8448) {
								error_print(25, line_num, names_table, objects_beg);
								return 1;
							}
							int21h = true;
							output_line = (char*)malloc(6 * sizeof(char));
							if (output_line)
								strcpy(output_line, "CD21");
						}
						else {
							error_print(26, line_num, names_table, objects_beg);
							return 1;
						}
					}
				}
				//Расчёт параметра директивы org
				else {
					if (directive_num == 1) {
						offset = define_value(input_line, position + 4, op_bit, code);
						if (offset != 1 && offset != 0) {
							error_print(27, line_num, names_table, objects_beg);
							return 1;
						}
						offset *= 256;
					}
					else {
						error_print(28, line_num, names_table, objects_beg);
						return 1;
					}
				}
			}
			//Опредление имени сегмента
			else if (directive_num == 0) {
				variable = define_var_inline(input_line, position);
				names_table = add_name(names_table, variable, 1, comands_table, code);
				if (code) {
					error_print(code + 16, line_num, names_table, objects_beg);
					return 1;
				}
			}
			else {
				error_print(29, line_num, names_table, objects_beg);
				return 1;
			}
		}
		//Добавление элемента в список объектного кода
		if (objects_end) {
			objects_end->next = create_object(input_line, output_line, variable, label, position);
			objects_end = objects_end->next;
		}
		else {
			objects_beg = create_object(input_line, output_line, variable, label, position);
			objects_end = objects_beg;
		}
	}
	//Если не было завершения программы
	if (!end) {
		error_print(31, line_num, names_table, objects_beg);
		return 1;
	}
	//Второй проход транслятора (Построение объектного кода команд, рассчёт смещений, определение адреса переменных)
	temp = objects_beg->next->next;
	line_num = 2;
	default_offset = offset;
	end = false;
	while (temp) {
		line_num++;
		input_line = temp->input_line;
		temp->offset = offset;
		//В данной строке определена метка
		if (temp->position) {
			name = find_name(names_table, temp->label);
			//Определение адреса метки
			name->address = offset;
		}
		//Если кроме метки в строке ничего нет
		if (strlen(temp->input_line) == (size_t)temp->position) {
			temp = temp->next;
			continue;
		}
		if (temp->output_line) {
			end = true;
			//Директива определения данных, опредление адреса переменной
			if (temp->variable) {
				name = find_name(names_table, temp->variable);
				//Сохранение адреса с разворотом байтов
				name->address = offset % 256 * 256 + offset / 256;
				temp->variable = NULL;
			}
			offset += strlen(temp->output_line) / 2;
		}
		//Поиск команды, если не конец программы
		else if (!end) {
			comand_num = define_comand(input_line, temp->position, comands_table);
			code = 0;
			switch (comand_num) {
				case 0:
					comand = comand_mov(input_line, temp->position, names_table, code);
					break;
				case 1:
					comand = comand_xchg(input_line, temp->position, names_table, code);
					break;
				case 2:
					comand = comand_dec(input_line, temp->position, names_table, code);
					break;
				case 3:
					comand = comand_loop(input_line, temp->position, names_table, code);
					break;
				case -1:
					error_print(30, line_num, names_table, objects_beg);
					return 1;
			}
			if (comand) {
				temp->output_line = comand->output_line;
				temp->variable = comand->variable;
				offset += strlen(temp->output_line) / 2;
			}
			//Обработка ошибок определения объектного кода команд
			else {
				if (code) {
					error_print(code, line_num, names_table, objects_beg);
					return 1;
				}
			}
		}
		temp = temp->next;
	}
	//Третий проход транслятора (Определение в объектном коде команд адреса переменных, вывод листинга в файл и объектного кода в другой файл)
	fprintf(object_file, "H %s %X\n", names_table->name, offset - default_offset);
	temp = objects_beg;
	line_num = 1;
	while (temp) {
		//Имеется объектный код
		if (temp->output_line) {
			//Если команда с переменной
			if (temp->variable) {
				//Все команды кроме LOOP
				if (strlen(temp->output_line) > 4) {
					char str[5];
					//Поиск имени в таблице имён
					name = find_name(names_table, temp->variable);
					if (name) {
						int pos = strstr(temp->output_line, "0000") - temp->output_line;
						sprintf(str, "%X", name->address);
						for (int j = 1; j <= (int)strlen(str); j++) {
							temp->output_line[pos + 4 - j] = str[strlen(str) - j];
						}
					}
				}
				//Команда LOOP
				else {
					name = find_name(names_table, temp->variable);
					number = name->address - temp->offset;
					number -= 2;
					sprintf(str, "%X", number);
					if (strlen(str) > 1)
						temp->output_line[2] = str[strlen(str) - 2];
					temp->output_line[3] = str[strlen(str) - 1];
				}
			}
			number = strlen(temp->output_line) / 2;
			fprintf(object_file, "T %X %X %s\n", temp->offset, number, temp->output_line);
			fprintf(listing_file, "[%4d]%8X:%-16s\t%s\n", line_num, temp->offset, temp->output_line, temp->input_line);
		}
		else if (temp->label)
			fprintf(listing_file, "[%4d]%8X:%-16s\t%s\n", line_num, temp->offset, "", temp->input_line);
		else
			fprintf(listing_file, "[%4d]%8s:%-16s\t%s\n", line_num, "", "", temp->input_line);
		line_num++;
		temp = temp->next;
	}
	fprintf(object_file, "E %X\n", default_offset);
	fclose(input_file);
	fclose(listing_file);
	fclose(object_file);
	//Очистка мусора
	garbage_clean(names_table, objects_beg);
	puts("Программа отработала успешно");
	return 0;
}
