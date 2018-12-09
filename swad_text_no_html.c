// swad_text_no_html.c: text messages on screen, in several languages
/*****************************************************************************
		  #   #  ###       #   # ##### #   # #
		  ##  # #   #      #   #   #   ## ## #
		  # # # #   #      #####   #   # # # #
		  #  ## #   #      #   #   #   #   # #
		  #   #  ###       #   #   #   #   # #####
******************************************************************************/
/**** Don't use HTML entities like &oacute; when variable ends in NO_HTML ****/
/****              Charset must be windows-1252 in this file              ****/
/*****************************************************************************/

/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2018 Antonio Ca�as Vargas

    Catal� translation:
	Antonio Ca�as Vargas
	Joan Llu�s D�az Rodr�guez
    German translation:
	Antonio Ca�as Vargas
	Rafael Barranco-Droege
    English translation (finished):
	Antonio Ca�as Vargas
    Spanish translation (finished):
	Antonio Ca�as Vargas
    French translation:
	Antonio Ca�as Vargas
    Guarani translation:
	Antonio Ca�as Vargas
    Italian translation (finished):
	Antonio Ca�as Vargas
	Nicola Comunale Rizzo
	Francisco Manuel Herrero P�rez
	Giuseppe Antonio Pagin
	Antonella Grande
    Polish translation:
	Antonio Ca�as Vargas
	Wojtek Kieca
	Tomasz Olechowski
	Mateusz Stanko
    Portuguese translation:
	Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************/
/********************************* Headers ***********************************/
/*****************************************************************************/

#include "swad_language.h"
#include "swad_notification.h"
#include "swad_user.h"

/*****************************************************************************/
/******************************** Constants **********************************/
/*****************************************************************************/

#ifndef L
#define L 3	// English
#endif

const char *Txt_Country_NO_HTML =
#if   L==1
	"Pa�s";
#elif L==2
	"Land";
#elif L==3
	"Country";
#elif L==4
	"Pa�s";
#elif L==5
	"Pays;";
#elif L==6
	"Teta";
#elif L==7
	"Paese";
#elif L==8
	"Kraj";
#elif L==9
	"Pa&iacute;s";
#endif

const char *Txt_Confirmation_of_your_email_NO_HTML =	// Don't use HTML entities like &egrave; here
#if   L==1
	"Confirmaci�n de su direcci�n de correo";	// Necessita traduccio
#elif L==2
	"Confirmation of your email";			// Need �bersetzung
#elif L==3
	"Confirmation of your email";
#elif L==4
	"Confirmaci�n de su direcci�n de correo";
#elif L==5
	"Confirmation of your email";			// Besoin de traduction
#elif L==6
	"Confirmaci�n de su direcci�n de correo";	// Okoteve traducci�n
#elif L==7
	"Confirmation of your email";			// Bisogno di traduzione
#elif L==8
	"Confirmation of your email";			// Potrzebujesz tlumaczenie
#elif L==9
	"Confirma��o do seu email.";
#endif

// The following variables are compilated together in all languages because they are used in emails...
// ...and each email is sent in the language of the recipient

const char *Txt_Course_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &egrave; here
	{
	"",
	"Assignatura",
	"Kurs",
	"Course",
	"Asignatura",
	"Mati�re",
	"Mbo'esyry",
	"Corso",
	"Kurs",
	"Disciplina",
	};

const char *Txt_Dear_NO_HTML[Usr_NUM_SEXS][1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &oacute; here
{
	{
	// Usr_SEX_UNKNOWN
	"",
	"Estimat/da",
	"Sehr geehrte/r",
	"Dear",
	"Estimado/a",
	"Cher/Ch�re",
	"Estimado/a",	// Okoteve traducci�n
	"Egregio/a",
	"Drogi",
	"Prezado/a",
	},
	{
	// Usr_SEX_FEMALE
	"",
	"Estimada",
	"Sehr geehrte",
	"Dear",
	"Estimada",
	"Ch�re",
	"Estimada",	// Okoteve traducci�n
	"Egregia",
	"Drogi",
	"Prezada",
	},
	{
	// Usr_SEX_MALE
	"",
	"Estimat",
	"Sehr geehrter",
	"Dear",
	"Estimado",
	"Cher",
	"Estimado",	// Okoteve traducci�n
	"Egregio",
	"Drogi",
	"Prezado",
	},
	{
	// Usr_SEX_ALL
	"",
	"Estimat/da",
	"Sehr geehrte/r",
	"Dear",
	"Estimado/a",
	"Cher/Ch�re",
	"Estimado/a",	// Okoteve traducci�n
	"Egregio/a",
	"Drogi",
	"Prezado/a",
	}
};

const char *Txt_Forum_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &oacute; here
	{
	"",
	"F�rum",
	"Forum",
	"Forum",
	"Foro",
	"Forum",
	"Foro",		// Okoteve traducci�n
	"Forum",
	"Forum",
	"F�rum",
	};

const char *Txt_General_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &eacute; here
	{
	"",
	"General",
	"Allgemein",
	"General",
	"General",
	"G�n�ral",
	"General",	// Okoteve traducci�n
	"Generale",
	"Og�lne",
	"Geral",
	};

const char *Txt_Go_to_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &agrave; here
	{
	"",
	"Anar a",
	"Gehe zum",
	"Go to",
	"Ir a",
	"Aller �",
	"Ir a",		// Okoteve traducci�n
	"Vai a",
	"Idz do",
	"Ir para",
	};

const char *Txt_If_you_just_request_from_X_the_confirmation_of_your_email_Y_NO_HTML =	// Don't use HTML entities like &egrave; here
#if   L==1
	"Si usted acaba de solicitar desde %s"
	" la confirmaci�n de su correo %s,"
	" pulse en el siguiente enlace"
	" para confirmar dicha direcci�n:"
	" %s/?act=%ld&key=%s\n\n"
	"Si no ha sido usted, �no pulse en el enlace anterior!"
	" En este caso, le recomendamos que compruebe en %s"
	" si ha confirmado su direcci�n de correo.\n\n";	// Necessita traduccio
#elif L==2
	"If you just request from %s"
	" the confirmation of your email %s,"
	" click on the following link"
	" to confirm that direction:"
	" %s/?act=%ld&key=%s\n\n"
	"If it was not you, do not click on the link above!"
	" In this case, we recommend that you check in %s"
	" if you have confirmed your email address.\n\n";	// Need �bersetzung
#elif L==3
	"If you just request from %s"
	" the confirmation of your email %s,"
	" click on the following link"
	" to confirm that direction:"
	" %s/?act=%ld&key=%s\n\n"
	"If it was not you, do not click on the link above!"
	" In this case, we recommend that you check in %s"
	" if you have confirmed your email address.\n\n";
#elif L==4
	"Si usted acaba de solicitar desde %s"
	" la confirmaci�n de su correo %s,"
	" pulse en el siguiente enlace"
	" para confirmar dicha direcci�n:"
	" %s/?act=%ld&key=%s\n\n"
	"Si no ha sido usted, �no pulse en el enlace anterior!"
	" En este caso, le recomendamos que compruebe en %s"
	" si ha confirmado su direcci�n de correo.\n\n";
#elif L==5
	"If you just request from %s"
	" the confirmation of your email %s,"
	" click on the following link"
	" to confirm that direction:"
	" %s/?act=%ld&key=%s\n\n"
	"If it was not you, do not click on the link above!"
	" In this case, we recommend that you check in %s"
	" if you have confirmed your email address.\n\n";	// Besoin de traduction
#elif L==6
	"Si usted acaba de solicitar desde %s"
	" la confirmaci�n de su correo %s,"
	" pulse en el siguiente enlace"
	" para confirmar dicha direcci�n:"
	" %s/?act=%ld&key=%s\n\n"
	"Si no ha sido usted, �no pulse en el enlace anterior!"
	" En este caso, le recomendamos que compruebe en %s"
	" si ha confirmado su direcci�n de correo.\n\n";	// Okoteve traducci�n
#elif L==7
	"If you just request from %s"
	" the confirmation of your email %s,"
	" click on the following link"
	" to confirm that direction:"
	" %s/?act=%ld&key=%s\n\n"
	"If it was not you, do not click on the link above!"
	" In this case, we recommend that you check in %s"
	" if you have confirmed your email address.\n\n";	// Bisogno di traduzione
#elif L==8
	"If you just request from %s"
	" the confirmation of your email %s,"
	" click on the following link"
	" to confirm that direction:"
	" %s/?act=%ld&key=%s\n\n"
	"If it was not you, do not click on the link above!"
	" In this case, we recommend that you check in %s"
	" if you have confirmed your email address.\n\n";	// Potrzebujesz tlumaczenie
#elif L==9
	"Se voc� acabou de solicitar de %s"
	" a confirma��o do seu email %s,"
	" clique no seguinte link para confirmar a dire��o:"
	" %s/?act=%ld&key=%s\n\n"
	"Se n�o foi voc�, n�o clique no link acima!"
	" Nesse caso, recomendamos que voc� verifique em %s"
	" se voc� confirmou seu endere�o de e-mail.\n\n";
#endif

const char *Txt_If_you_no_longer_wish_to_receive_email_notifications_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &egrave; here
	{
	"",
	"Si no desitgeu rebre m�s avisos per correu,"
	" desactiveu la opci� corresponent en les prefer�ncies del seu perfil.",
	"Wenn Sie nicht mehr erhalten m�chten, E-Mail-Benachrichtigungen,"
	" Deaktivieren Sie die Option in den Einstellungen Ihres Profils.",
	"If you no longer wish to receive email notifications,"
	" uncheck the option in the preferences of your profile.",
	"Si no desea recibir m�s avisos por correo,"
	" desactive la opci�n correspondiente en las preferencias de su perfil.",
	"Si vous ne souhaitez plus recevoir de notifications par email,"
	" d�cochez l'option dans les pr�f�rences de votre profil.",
	"Si no desea recibir m�s avisos por correo,"
	" desactive la opci�n correspondiente en las preferencias de su perfil.",	// Okoteve traducci�n
	"Se non desideri pi� ricevere notifiche via email,"
	" deselezionare l'opzione nelle preferenze del tuo profilo.",
	"Jesli nie chcesz otrzymywac powiadomien email,"
	" usu&nacute; zaznaczenie opcje w ustawieniach swojego profilu.",
	"Se j� n�o desejar receber notifica��es por email,"
	" desmarque a op��o nas prefer�ncias do seu perfil.",
	};

const char *Txt_Institutions_NO_HTML =
#if   L==1
	"Institucions";
#elif L==2
	"Hochschulen";
#elif L==3
	"Institutions";
#elif L==4
	"Instituciones";
#elif L==5
	"�tablissements";
#elif L==6
	"Mbo'ehao";
#elif L==7
	"Istituzioni";
#elif L==8
	"Instytucje";
#elif L==9
	"Institu��es";
#endif

const char *Txt_MSG_From_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &egrave; here
	{
	"",
	"De",
	"Von",
	"From",
	"De",
	"De",
	"De",	// Okoteve traducci�n
	"Da",
	"Od",
	"De",
	};

const char *Txt_Notifications_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &egrave; here
	{
	"",
	"Notificacions",
	"Anmeldungen",
	"Notifications",
	"Notificaciones",
	"Notifications",
	"Momarandu",
	"Notifiche",
	"Powiadomienia",
	"Notifica��es",
	};

const char *Txt_New_password_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &ntilde; here.
	{
	"",
	"Nova contrasenya",
	"Neues Passwort",
	"New password",
	"Nueva contrase�a",
	"Nouveau mot de passe",
	"Nueva contrase�a",	// Okoteve traducci�n
	"Nuova password",
	"Nowe has&lstrok;o",
	"Nova senha",
	};

const char *Txt_NOTIFY_EVENTS_SINGULAR_NO_HTML[Ntf_NUM_NOTIFY_EVENTS][1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &uuml; here.
{
	{
	// Ntf_EVENT_UNKNOWN
	"",
	"Esdeveniment desconegut",
	"Unbekannt Ereignis",
	"Unknown event",
	"Evento desconocido",
	"�v�nement inconnu",
	"Evento desconocido",			// Okoteve traducci�n
	"Evento sconosciuto",
	"Nieznane zdarzenia",
	"Evento desconhecido",
	},
	{
	// Ntf_EVENT_DOCUMENT_FILE
	"",
	"Nou document",
	"Neue Dokumentdatei",
	"New document file",
	"Nuevo documento",
	"Nouveau fichier de document",
	"Nuevo documento",			// Okoteve traducci�n
	"Nuovo documento",
	"Nowy plik dokumentu",
	"Novo arquivo de documento",
	},
	{
	// Ntf_EVENT_TEACHERS_FILE
	"",
	"Nou arxiu de professors",
	"Neue Lehrer Datei",
	"New teachers' file",
	"Nuevo archivo de profesores",
	"Nouveau fichier d'enseignants",
	"Nuevo archivo de profesores",		// Okoteve traducci�n
	"Nuovo file di professori",
	"Nowy Nauczyciela plik",
	"Novo arquivo dos professores",
	},
	{
	// Ntf_EVENT_SHARED_FILE
	"",
	"Nou arxiu compartit",
	"Neue freigegebene Datei",
	"New shared file",
	"Nuevo archivo compartido",
	"Nouveau fichier partag�",
	"Nuevo archivo compartido",		// Okoteve traducci�n
	"Nuovo file condiviso",
	"Nowy Udostepniony plik",
	"Novo arquivo compartilhado",
	},
	{
	// Ntf_EVENT_ASSIGNMENT
	"",
	"Nova activitat",
	"Neue Aufgabe",
	"New assignment",
	"Nueva actividad",
	"Nouvelle activit�",
	"Nueva actividad",			// Okoteve traducci�n
	"Nuova attivit�",
	"Nowe zadania",
	"Nova atividade",
	},
	{
	// Ntf_EVENT_EXAM_ANNOUNCEMENT
	"",
	"Nova convocat�ria d'examen",
	"Neue Aufrufe f�r Pr�fung",
	"New announcement of exam",
	"Nueva convocatoria de examen",
	"Nouvelle convocation � un examen",
	"Nueva convocatoria de examen",		// Okoteve traducci�n
	"Nuovo appello d'esame",
	"Nowe ogloszenie egzaminu",
	"Nova chamada para exame",
	},
	{
	// Ntf_EVENT_MARKS_FILE
	"",
	"Nou fitxer de calificacions",
	"Neue Datei mit Bewertung",
	"New file with marks",
	"Nuevo archivo de calificaciones",
	"Nouveau fichier avec des notes",
	"Nuevo archivo de calificaciones",	// Okoteve traducci�n
	"Nuovo file con i risultati",
	"Nowy plik ze znakami",
	"Novo arquivo de notas",
	},
	{
	// Ntf_EVENT_ENROLMENT_STD
	"",
	"Nova inscripci� com a estudiant",
	"New Einschreibung als Student",
	"New enrolment as a student",
	"Nueva inscripci�n como estudiante",
	"Nouvelle inscriptionen tant qu'�tudiant",
	"Nueva inscripci�n como estudiante",			// Okoteve traducci�n
	"Nuova iscrizione come studente",
	"Nowe zapisy jako student",
	"Nova inscri��o como estudante",
	},
	{
	// Ntf_EVENT_ENROLMENT_TCH
	"",
	"Nova inscripci� com a professor/a",
	"New Einschreibung als Lehrkraft",
	"New enrolment as a teacher",
	"Nueva inscripci�n como profesor/a",
	"Nouvelle inscription en tant qu'enseignant",
	"Nueva inscripci�n como profesor/a",			// Okoteve traducci�n
	"Nuova iscrizione come professore/ssa",
	"Nowe zapisy jako nauczyciel",
	"Nova inscri��o como professor/a",
	},
	{
	// Ntf_EVENT_ENROLMENT_REQUEST
	"",
	"Nova petici� d'inscripci�",
	"New Registrierungsanforderung",
	"New enrolment request",
	"Nueva petici�n de inscripci�n",
	"Nouvelle demande d'inscription",
	"Nueva petici�n de inscripci�n",	// Okoteve traducci�n
	"Nuova richiesta di iscrizione",
	"Nowe prosby o rejestracji",
	"Novo pedido de inscri��o",
	},
	{
	// Ntf_EVENT_TIMELINE_COMMENT
	"",
	"Nou comentari a un missatge social",
	"Neue Kommentar zu soziale Beitrag",
	"New comment to social post",
	"Nuevo comentario a un mensaje social",
	"Nouveau commentaire � message social",
	"Nuevo comentario a un mensaje social",			// Okoteve traducci�n
	"Nuovo commento a un post sociale",
	"Nowy komentarz do post spoleczna",
	"Novo coment�rio a um post sociais",
	},
	{
	// Ntf_EVENT_TIMELINE_FAV
	"",
	"Nou favorit a un missatge social",
	"Neue Favorit zu soziale Beitrag",
	"New favourite to social post",
	"Nuevo favorito a un mensaje social",
	"Nouveau favori � message social",
	"Nuevo favorito a un mensaje social",			// Okoteve traducci�n
	"Nuovo preferito a un post sociale",
	"Nowy ulubiony do post spoleczna",
	"Novo favorito a um post sociais",
	},
	{
	// Ntf_EVENT_TIMELINE_SHARE
	"",
	"Nova compartici� de missatge social",
	"Neue gemeinsame Nutzung von soziale Beitrag",
	"New sharing of social post",
	"Nueva compartici�n de mensaje social",
	"Nouveau partage de message social",
	"Nueva compartici�n de mensaje social",			// Okoteve traducci�n
	"Nuova condivisione di post sociale",
	"Nowy podzial post spoleczna",
	"Nova partilha de post sociais",
	},
	{
	// Ntf_EVENT_TIMELINE_MENTION
	"",
	"Nova menci�",
	"Neue Erw�hnung",
	"New mention",
	"Nueva menci�n",
	"Nouveau mention",
	"Nueva menci�n",			// Okoteve traducci�n
	"Nuovo menzione",
	"Nowa wzmianka",
	"Nova men��o",
	},
	{
	// Ntf_EVENT_FOLLOWER
	"",
	"Nou seguidor",
	"Neue Anh&auml;nger",
	"New follower",
	"Nuevo seguidor",
	"Nouveau suiveur",
	"Nuevo seguidor",	// Okoteve traducci�n
	"Nuovo follower",
	"Nowy obserwuj&aogon;",
	"Novo seguidor",
	},
	{
	// Ntf_EVENT_FORUM_POST_COURSE
	"",
	"Novo comentari en un f�rum",
	"Neue Nachricht in einem Forum",
	"New post in a forum",
	"Nuevo comentario en un foro",
	"Nouveau post dans un forum",
	"Nuevo comentario en un foro",		// Okoteve traducci�n
	"Nuovo post in un forum",
	"Nowy post na forum",
	"Novo post em um f�rum",
	},
	{
	// Ntf_EVENT_FORUM_REPLY
	"",
	"Nova resposta en un f�rum",
	"Neue Antwort in einem Forum",
	"New reply in a forum",
	"Nueva respuesta en un foro",
	"Nouvelle r�ponse dans un forum",
	"Nueva respuesta en un foro",		// Okoteve traducci�n
	"Nuova risposta in un forum",
	"Odpowiedz nowe na forum",
	"Nova resposta em um f�rum",
	},
	{
	// Ntf_EVENT_NOTICE
	"",
	"Nou av�s",
	"Neuen Ank�ndigungen",
	"New notice",
	"Nuevo aviso",
	"Nouvel avis",
	"Nuevo aviso",				// Okoteve traducci�n
	"Nuovo avviso",
	"Nowe obwieszczenie",
	"Novo an�ncio",
	},
	{
	// Ntf_EVENT_MESSAGE
	"",
	"Nou missatge",
	"Neue Nachricht",
	"New message",
	"Nuevo mensaje",
	"Nouveau message",
	"Nuevo mensaje",			// Okoteve traducci�n
	"Nuovo messaggio",
	"Nowa wiadomosc",
	"Nova mensagem",
	},
	{
	// Ntf_EVENT_SURVEY		// TODO: Move to assessment tab (also necessary in database) !!!!!!!!!
	"",
	"Nova enquesta",
	"Neuer Umfrage",
	"New survey",
	"Nueva encuesta",
	"Nouveau sondage",
	"Nueva encuesta",			// Okoteve traducci�n
	"Nuovo sondaggio",
	"Nowe badania",
	"Novo inqu&eacute;rito",
	},
	{
	// Ntf_EVENT_ENROLMENT_NET	// TODO: Move to users tab (also necessary in database) !!!!!!!!!
	"",
	"Nova inscripci� com a professor/a no editor/a",
	"New Einschreibung als nicht bearbeiteter Lehrkraft",
	"New enrolment as a non-editing teacher",
	"Nueva inscripci�n como profesor/a no editor/a",
	"Nouvelle inscription en tant qu'enseignant/e non-�diteur/trice",
	"Nueva inscripci�n como profesor/a no editor/a",	// Okoteve traducci�n
	"Nuova iscrizione come professore/ssa non-editing",
	"Nowe zapisy jako nauczyciel nie edytujacy",
	"Nova inscri��o como professor/a n�o editor/a",
	},
};

const char *Txt_NOTIFY_EVENTS_There_is_a_new_event_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Warning: it is very important to include %s in the following sentences
	{			// Don't use HTML entities like &uuml; here.
	"",
	"Hi ha un nou esdeveniment en %s",
	"Es ist eine neue Ereignis in %s",
	"There is a new event in %s",
	"Hay un nuevo evento en %s",
	"Il y un nouvel �v�nement dans %s",
	"Hay un nuevo evento en %s",	// Okoteve traducci�n
	"C'� un nuovo evento in %s",
	"Jest nowe wydarzenie w %s",
	"Existe um novo evento em %s",
	};

const char *Txt_NOTIFY_EVENTS_There_are_X_new_events_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Warning: it is very important to include %u and %s in the following sentences
	{			// Don't use HTML entities like &uuml; here.
	"",
	"Hi ha %u nous esdeveniments en %s",
	"Es gibt %u neue Ereignisse in %s",
	"There are %u new events in %s",
	"Hay %u nuevos eventos en %s",
	"Il y a %u nouveaux �v�nements dans %s",
	"Hay %u nuevos eventos en %s",	// Okoteve traducci�n
	"Ci sono %u nuovi eventi in %s",
	"Istnieje %u nowych wydarzen w %s",
	"H� %u novos eventos em %s",
	};

const char *Txt_only_teachers_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &oacute; here
	{
	"",
	", professors",
	", Lehrkr�fte",
	", teachers",
	", profesores",
	", enseignants",
	", profesores",	// Okoteve traducci�n
	", professori",
	", nauczyciel",
	", professores",
	};

const char *Txt_Please_do_not_reply_to_this_automatically_generated_email_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &egrave; here
	{
	"",
	"Si us plau, no respongui a aquest correu generat autom�ticament.",
	"Bitte, antworten Sie nicht auf diese automatisch generierte E-Mail.",
	"Please do not reply to this automatically generated email.",
	"Por favor, no responda a este correo generado autom�ticamente.",
	"S'il vous pla�t, ne pas r�pondre � ce courrier �lectronique g�n�r� automatiquement.",
	"Por favor, no responda a este correo generado autom�ticamente.",	// Okoteve traducci�n
	"Per favore, non rispondere a questa email generata automaticamente.",
	"Prosimy nie odpowiadac na to automatycznie wygenerowany email.",
	"Por favor, n�o responda a este email gerado automaticamente.",
	 };

const char *Txt_TAB_Messages_NO_HTML[1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &egrave; here
	{
	"",
	"Missatges",
	"Nachrichten",
	"Messages",
	"Mensajes",
	"Messages",
	"Marandu",
	"Messaggi",
	"Wiadomosci",
	"Mensagens",
	};

const char *Txt_The_following_password_has_been_assigned_to_you_to_log_in_X_NO_HTML =	// Warning: it is very important to include %s and %u in the following sentences. Don't use HTML entities like &egrave; here
#if   L==1
	"Se li ha assignat la seg�ent contrasenya per entrar a %s: %s\n"
	"Si desitja activar aquesta nova contrasenya,"
	" ha de entrar a %s amb el seu ID (DNI/c�dula), sobrenom o email"
	" i aquesta nova contrasenya abans de %u dies."
	" Un cop hagi entrat, pot canviar la contrasenya per una altra.\n"
	"Si no accedeix amb aquesta nova contrasenya,"
	" seguir� tenint la contrasenya que tingu�s fins ara.\n"
	"Si ha rebut aquest missatge sense sol&middot;licitar-lo,"
	" es deu a que alg� que coneix el vostre ID (DNI/c�dula), el vostre sobrenom o el vostre email"
	" ha sol&middot;licitat l'enviament de una nova contrasenya per a vost�.\n"
	"Aquesta nova contrasenya se ha enviat nom�s a la adre�a de correu %s,"
	" que �s la que apareix a la seva fitxa.\n\n";
#elif L==2
	"The following password has been assigned to you to log in %s: %s\n"
	"If you want to activate the new password,"
	" you must enter %s with your ID/nickname/email"
	" and this new password before %u days."
	" Once you have logged in, you can change your password.\n"
	"If you do not log in with this new password,"
	" the old password will remain valid.\n"
	"If you have received this message without having requested it,"
	" is that someone who knows your ID, nickname or email has requested a new password sent to you.\n"
	"This new password has been sent only to the email address %s,"
	" which is on your record card.\n\n";	// Need �bersetzung
#elif L==3
	"The following password has been assigned to you to log in %s: %s\n"
	"If you want to activate the new password,"
	" you must enter %s with your ID/nickname/email"
	" and this new password before %u days."
	" Once you have logged in, you can change your password.\n"
	"If you do not log in with this new password,"
	" the old password will remain valid.\n"
	"If you have received this message without having requested it,"
	" is that someone who knows your ID, nickname or email has requested a new password sent to you.\n"
	"This new password has been sent only to the email address %s,"
	" which is on your record card.\n\n";
#elif L==4
	"Se le ha asignado la siguiente contrase�a para entrar en %s: %s\n"
	"Si desea activar esta nueva contrase�a,"
	" debe entrar en %s con su ID (DNI/c�dula), apodo o direcci�n de correo"
	" y esta nueva contrase�a antes de %u d�as."
	" Una vez que haya entrado, puede cambiar la contrase�a por otra.\n"
	"Si no accede con esta nueva contrase�a,"
	" seguir� teniendo la contrase�a que tuviera hasta ahora.\n"
	"Si ha recibido este mensaje sin solicitarlo,"
	" se debe a que alguien que conoce su ID (DNI/c�dula), su apodo o su direcci�n de correo"
	" ha solicitado el env�o de una nueva contrase�a para usted.\n"
	"Esta nueva contrase�a se ha enviado s�lo a la direcci�n de correo %s,"
	" que es la que aparece en su ficha.\n\n";
#elif L==5
	"The following password has been assigned to you to log in %s: %s\n"
	"If you want to activate the new password,"
	" you must enter %s with your ID/nickname/email"
	" and this new password before %u days."
	" Once you have logged in, you can change your password.\n"
	"If you do not log in with this new password,"
	" the old password will remain valid.\n"
	"If you have received this message without having requested it,"
	" is that someone who knows your ID, nickname or email has requested a new password sent to you.\n"
	"This new password has been sent only to the email address %s,"
	" which is on your record card.\n\n";	// Besoin de traduction
#elif L==6
	"Se le ha asignado la siguiente contrase�a para entrar en %s: %s\n"
	"Si desea activar esta nueva contrase�a,"
	" debe entrar en %s con su ID (DNI/c&eacute;dula), apodo o direcci�n de correo"
	" y esta nueva contrase�a antes de %u d�as."
	" Una vez que haya entrado, puede cambiar la contrase�a por otra.\n"
	"Si no accede con esta nueva contrase�a,"
	" seguir� teniendo la contrase�a que tuviera hasta ahora.\n"
	"Si ha recibido este mensaje sin solicitarlo,"
	" se debe a que alguien que conoce su ID (DNI/c�dula), su apodo o su direcci�n de correo"
	" ha solicitado el env�o de una nueva contrase�a para usted.\n"
	"Esta nueva contrase�a se ha enviado s�lo a la direcci�n de correo %s,"
	" que es la que aparece en su ficha.\n\n";	// Okoteve traducci�n
#elif L==7
	"La seguente password ti � stata assegnata per entrare in %s: %s\n"
	"Se vuoi attivare la nuova password,"
	" devi entrare in %s con il tuo ID / nome utente / email"
	" e li c'� la nuova password prima di %u giorni."
	" Una volta che ti sei loggato, puoi scegliere la tua password.\n"
	"Se tu non entri con questa nuova password,"
	" rimarr� valida la vecchia password.\n"
	"Se hai ricevuto questo messaggio senza averlo richiesto,"
	" � perch� qualcuno che sa il tuo ID, nome utente o email ha richiesto una nuova password da inviarti.\n"
	"Questa nuova password � stata inviata solo all'indirizzo email %s,"
	" che � presente sulla tua scheda.\n\n";
#elif L==8
	"The following password has been assigned to you to log in %s: %s\n"
	"If you want to activate the new password,"
	" you must enter %s with your ID/nickname/email"
	" and this new password before %u days."
	" Once you have logged in, you can change your password.\n"
	"If you do not log in with this new password,"
	" the old password will remain valid.\n"
	"If you have received this message without having requested it,"
	" is that someone who knows your ID, nickname or email has requested a new password sent to you.\n"
	"This new password has been sent only to the email address %s,"
	" which is on your record card.\n\n";	// Potrzebujesz tlumaczenie
#elif L==9
	"A seguinte senha foi atribu�da a voc� para efetuar login em %s: %s\n"
	"Se voc� quiser ativar a nova senha,"
	" voc� deve digitar %s com seu ID/alcunha/e-mail"
	" e esta nova senha antes de %u dias."
	" Depois de ter logado, voc� pode alterar sua senha.\n"
	"Se voc� n�o fizer login com essa nova senha,"
	" a senha antiga permanecer� v�lida.\n"
	"Se voc� recebeu esta mensagem sem solicit�-la,"
	" algu�m que conhece seu ID, alcunha ou e-mail solicitou uma nova senha enviada para voc�.\n"
	"Esta nova senha foi enviada apenas para o endere�o de e-mail %s,"
	" que est� no seu cart�o de registro.\n\n";
#endif

const char *Txt_Unknown_or_without_photo =
#if   L==1
	"Desconocido/a o sin fotograf&iacute;a";	// Necessita traduccio
#elif L==2
	"Unknown or without photo";			// Need �bersetzung
#elif L==3
	"Unknown or without photo";
#elif L==4
	"Desconocido/a o sin fotograf&iacute;a";
#elif L==5
	"Unknown or without photo";			// Besoin de traduction
#elif L==6
	"Desconocido/a o sin fotograf&iacute;a";	// Okoteve traducci�n
#elif L==7
	"Sconosciuto/a o senza foto";
#elif L==8
	"Unknown or without photo";			// Potrzebujesz tlumaczenie
#elif L==9
	"Desconhecido/a ou sem foto";
#endif

const char *Txt_UPLOAD_FILE_could_not_create_file_NO_HTML =	// Warning: it is very important to include %s in the following sentences
#if   L==1
	"%s: no s'ha pogut crear el fitxer.";
#elif L==2
	"%s: Datei konnte nicht erstellt werden.";
#elif L==3
	"%s: could not create file.";
#elif L==4
	"%s: no se ha podido crear el archivo.";
#elif L==5
	"%s: impossible de cr�er le fichier.";
#elif L==6
	"%s: no se ha podido crear el archivo.";	// Okoteve traducci�n
#elif L==7
	"%s: impossibile creare il file.";
#elif L==8
	"%s: nie mozna utworzyc plik.";
#elif L==9
	"%s: n�o foi poss�vel criar arquivo.";
#endif

const char *Txt_UPLOAD_FILE_Forbidden_NO_HTML =
#if   L==1
	"No puede crear archivos aqu�.";	// Necessita traduccio
#elif L==2
	"You can not create files here.";	// Need �bersetzung
#elif L==3
	"You can not create files here.";
#elif L==4
	"No puede crear archivos aqu�.";
#elif L==5
	"You can not create files here.";	// Besoin de traduction
#elif L==6
	"No puede crear archivos aqu�.";	// Okoteve traducci�n
#elif L==7
	"Non puoi creare file qui.";
#elif L==8
	"You can not create files here.";	// Potrzebujesz tlumaczenie
#elif L==9
	"Voc� n�o pode criar arquivos aqui.";
#endif

const char *Txt_UPLOAD_FILE_You_must_specify_the_file_NO_HTML =
#if   L==1
	"Debe especificar el archivo.";	// Necessita traduccio
#elif L==2
	"Die Datei muss angegeben werden.";
#elif L==3
	"You must specify the file.";
#elif L==4
	"Debe especificar el archivo.";
#elif L==5
	"Vous devez sp�cifier le fichier.";
#elif L==6
	"Debe especificar el archivo.";	// Okoteve traducci�n
#elif L==7
	"� necessario specificare il file.";
#elif L==8
	"You must specify the file.";	// Potrzebujesz tlumaczenie
#elif L==9
	"Voc� deve especificar o arquivo.";
#endif

const char *Txt_UPLOAD_FILE_X_extension_not_allowed_NO_HTML =	// Warning: it is very important to include %s in the following sentences
#if   L==1
	"%s: extensi�n no permitida.";		// Necessita traduccio
#elif L==2
	"%s: Endung nicht unterst�tzt.";
#elif L==3
	"%s: extension not allowed.";
#elif L==4
	"%s: extensi�n no permitida.";
#elif L==5
	"%s: extension not allowed.";		// Besoin de traduction
#elif L==6
	"%s: extensi�n no permitida.";		// Okoteve traducci�n
#elif L==7
	"%s: estensione non permessa.";
#elif L==8
	"%s: extension not allowed.";		// Potrzebujesz tlumaczenie
#elif L==9
	"%s: extens�o n�o permitida.";
#endif

const char *Txt_UPLOAD_FILE_X_file_already_exists_NO_HTML =	// Warning: it is very important to include %s in the following sentences
#if   L==1
	"%s: el archivo ya existe.";	// Necessita traduccio
#elif L==2
	"%s: file already exists.";	// Need �bersetzung
#elif L==3
	"%s: file already exists.";
#elif L==4
	"%s: el archivo ya existe.";
#elif L==5
	"%s: file already exists.";	// Besoin de traduction
#elif L==6
	"%s: el archivo ya existe.";	// Okoteve traducci�n
#elif L==7
	"%s: file already exists.";	// Bisogno di traduzione
#elif L==8
	"%s: file already exists.";	// Potrzebujesz tlumaczenie
#elif L==9
	"%s: o arquivo j� existe.";
#endif

const char *Txt_UPLOAD_FILE_File_too_large_maximum_X_MiB_NO_HTML =	// Warning: it is very important to include %lu in the following sentences
#if   L==1
	"Archivo demasiado grande (m�ximo %lu MiB) /"
	" un altre problema d'enviament.";	// Necessita traduccio
#elif L==2
	"Datei zu gro� (Maximum %lu MiB) /"
	" ein weiteres Upload-Problem.";
#elif L==3
	"File too large (maximum %lu MiB) /"
	" another upload problem.";
#elif L==4
	"Archivo demasiado grande (m�ximo %lu MiB) /"
	" otro problema de env�o.";
#elif L==5
	"Fichier est trop gros (maximum %lu MiB) /"
	" un autre probl�me de t�l�chargement.";
#elif L==6
	"Archivo demasiado grande (m�ximo %lu MiB) /"
	" otro problema de env�o.";		// Okoteve traducci�n
#elif L==7
	"File troppo grande (massimo %lu MiB) /"
	" un altro problema di caricamento.";
#elif L==8
	"Plik jest za duzy (maksimum %lu MiB) /"
	" kolejny problem wgrac.";
#elif L==9
	"Arquivo muito grande (m�ximo %lu MiB) /"
	" outro problema de upload.";
#endif

const char *Txt_UPLOAD_FILE_Invalid_name_NO_HTML =	// Warning: it is very important to include %s in the following sentences
#if   L==1
	"Nombre no v�lido.";	// Necessita traduccio
#elif L==2
	"Invalid name.";	// Need �bersetzung
#elif L==3
	"Invalid name.";
#elif L==4
	"Nombre no v�lido.";
#elif L==5
	"Invalid name.";	// Besoin de traduction
#elif L==6
	"Nombre no v�lido.";	// Okoteve traducci�n
#elif L==7
	"Nome non valido.";
#elif L==8
	"Invalid name.";	// Potrzebujesz tlumaczenie
#elif L==9
	"Nome inv�lido.";
#endif

const char *Txt_UPLOAD_FILE_X_invalid_name_NO_HTML =	// Warning: it is very important to include %s in the following sentences
#if   L==1
	"%s: nombre no v�lido.";	// Necessita traduccio
#elif L==2
	"%s: invalid name.";		// Need �bersetzung
#elif L==3
	"%s: invalid name.";
#elif L==4
	"%s: nombre no v�lido.";
#elif L==5
	"%s: invalid name.";		// Besoin de traduction
#elif L==6
	"%s: nombre no v�lido.";	// Okoteve traducci�n
#elif L==7
	"%s: nome non valido.";
#elif L==8
	"%s: invalid name.";		// Potrzebujesz tlumaczenie
#elif L==9
	"%s: nome inv�lido.";
#endif

const char *Txt_UPLOAD_FILE_X_MIME_type_Y_not_allowed_NO_HTML =	// Warning: it is very important to include two %s in the following sentences
#if   L==1
	"%s: tipo MIME %s no permitido.";	// Necessita traduccio
#elif L==2
	"%s: MIME-Typ %s nicht unterst�tzt.";
#elif L==3
	"%s: MIME type %s not allowed.";
#elif L==4
	"%s: tipo MIME %s no permitido.";
#elif L==5
	"%s: MIME type %s not allowed.";	// Besoin de traduction
#elif L==6
	"%s: tipo MIME %s no permitido.";	// Okoteve traducci�n
#elif L==7
	"%s: tipo MIME %s non consentito.";
#elif L==8
	"%s: MIME type %s not allowed.";	// Potrzebujesz tlumaczenie
#elif L==9
	"%s: Tipo MIME %s n�o permitido.";
#endif

const char *Txt_UPLOAD_FILE_X_not_HTML_NO_HTML =	// Warning: it is very important to include %s in the following sentences
#if   L==1
	"%s: no HTML.";		// Necessita traduccio
#elif L==2
	"%s: not HTML.";	// Need �bersetzung
#elif L==3
	"%s: not HTML.";
#elif L==4
	"%s: no HTML.";		// Bisogno di traduzione
#elif L==5
	"%s: not HTML.";	// Besoin de traduction
#elif L==6
	"%s: no HTML.";		// Okoteve traducci�n
#elif L==7
	"%s: not HTML.";
#elif L==8
	"%s: not HTML.";	// Potrzebujesz tlumaczenie
#elif L==9
	"%s: n�o HTML.";
#endif

const char *Txt_UPLOAD_FILE_X_quota_exceeded_NO_HTML =	// Warning: it is very important to include %s in the following sentences
#if   L==1
	"%s: cuota excedida.";		// Necessita traduccio
#elif L==2
	"%s: Kontingent �berschritten.";
#elif L==3
	"%s: quota exceeded.";
#elif L==4
	"%s: cuota excedida.";
#elif L==5
	"%s: quota d�pass�.";
#elif L==6
	"%s: cuota excedida.";		// Okoteve traducci�n
#elif L==7
	"%s: quota superata.";
#elif L==8
	"%s: przekroczono przydzial.";
#elif L==9
	"5s: quota excedida.";
#endif

const char *Txt_UPLOAD_FILE_Upload_time_too_long_maximum_X_minutes_NO_HTML =	// Warning: it is very important to include %lu in the following sentences
#if   L==1
	"Tiempo de subida demasiado largo (m�ximo %lu minutos).";	// Necessita traduccio
#elif L==2
	"Upload ist zu lang (Maximum %lu Minute).";
#elif L==3
	"Upload time too long (maximum %lu minutes).";
#elif L==4
	"Tiempo de subida demasiado largo (m�ximo %lu minutos).";
#elif L==5
	"Temps de t�l�chargement trop longs (maximum %lu minutes).";
#elif L==6
	"Tiempo de subida demasiado largo (m�ximo %lu minutos).";	// Okoteve traducci�n
#elif L==7
	"Durata del caricamento troppo lungo (massimo %lu minuti).";
#elif L==8
	"Upload time too long (maximum %lu minuty).";			// Potrzebujesz tlumaczenie
#elif L==9
	"Tempo de upload muito longo (m�ximo %lu minutos).";
#endif

const char *Txt_user_NO_HTML[Usr_NUM_SEXS][1 + Lan_NUM_LANGUAGES] =	// Don't use HTML entities like &egrave; here
{
	{
	// Usr_SEX_UNKNOWN
	"",
	"usuari/a",
	"Benutzer",
	"user",
	"usuario/a",
	"utilisateur/se",
	"puruh�ra",
	"utente",
	"uzytkownika",
	"utilizador/a",
	},
	{
	// Usr_SEX_FEMALE
	"",
	"usuaria",
	"Benutzer",
	"user",
	"usuaria",
	"utilisateuse",
	"usuaria",	// Okoteve traducci�n
	"utente",
	"uzytkownika",
	"utilizadora",
	},
	{
	// Usr_SEX_MALE
	"",
	"usuari",
	"Benutzer",
	"user",
	"usuario",
	"utilisateur",
	"puruh�ra",
	"utente",
	"uzytkownika",
	"utilizador",
	},
	{
	// Usr_SEX_ALL
	"",
	"usuari/a",
	"Benutzer",
	"user",
	"usuario/a",
	"utilisateur/se",
	"puruh�ra",
	"utente",
	"uzytkownika",
	"utilizador/a",
	}
};

const char *Txt_Users_NO_HTML =
#if   L==1
	"Usuaris";
#elif L==2
	"Benutzer";
#elif L==3
	"Users";
#elif L==4
	"Usuarios";
#elif L==5
	"Utilisateurs";
#elif L==6
	"Puruh&aacute;ra";
#elif L==7
	"Utenti";
#elif L==8
	"Uzytkownik�w";
#elif L==9
	"Utilizadores";
#endif
