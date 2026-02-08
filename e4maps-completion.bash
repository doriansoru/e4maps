#!/usr/bin/env bash

_e4maps_completions()
{
    local cur prev words cword
    # Prova a inizializzare usando le funzioni standard di bash-completion
    if declare -F _init_completion >/dev/null 2>&1; then
        _init_completion || return
    else
        # Fallback manuale
        cur="${COMP_WORDS[COMP_CWORD]}"
        prev="${COMP_WORDS[COMP_CWORD-1]}"
    fi

    # Gestione opzioni (flag che iniziano con -)
    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "--help -h --convert-to --auto-layout" -- "${cur}") )
        return 0
    fi

    # Gestione argomenti dei flag
    case "${prev}" in
        --convert-to)
            COMPREPLY=( $(compgen -W "pdf png mm" -- "${cur}") )
            return 0
            ;;
    esac

    # Completamento file
    # Se la funzione _filedir esiste (standard in bash-completion), usala.
    # Gestisce perfettamente spazi, directory e quoting.
    if declare -F _filedir >/dev/null 2>&1; then
        _filedir 'e4m'
        return 0
    fi

    # Fallback robusto per ambienti senza bash-completion completo
    # Imposta IFS a newline per gestire i file con spazi
    local IFS=$'\n'
    compopt -o filenames 2>/dev/null
    # Genera lista di file .e4m e directory
    # Nota: questo fallback è meno potente di _filedir ma funziona per casi base
    COMPREPLY=( $(compgen -f -X "!*.e4m" -- "${cur}") )
}

complete -F _e4maps_completions e4maps